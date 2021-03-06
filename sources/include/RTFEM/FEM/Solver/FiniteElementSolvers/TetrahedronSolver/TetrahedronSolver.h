#ifndef PROJECT_TETRAHEDRONSOLVER_H
#define PROJECT_TETRAHEDRONSOLVER_H

#include <RTFEM/DataTypes.h>
#include <RTFEM/FEM/Solver/FiniteElementSolver.h>
#include "RTFEM/FEM/Solver/FiniteElementSolvers/TetrahedronSolver/TetrahedronSolverData.h"

#include <memory>

namespace rtfem {

template<class T>
class Vertex;

template<class T>
class TetrahedronFiniteElement;

/**
 * Solver for Linear Tetrahedron (constant gradient of shape function).
 * The geometry matrix B is constant with respect to X
 * (and always will be constant, independent of material/strain equations).
 * The simplicity of Linear Tetrahedron allows for analytical integration.
 *
 * Ordering:
 *  The ordering of Vertices within FiniteElement matters.
 *  The ordering of TriangleFaces within FiniteElement does not matter.
 *  The order of TriangleFaces are based on Vertices and predefined rules:
 *      Let vertices have indices:
 *          v1, v2, v3, v4.
 *      Then:
 *          Face1 = {v2, v3, v4},
 *          Face2 = {v3, v4, v1},
 *          Face3 = {v4, v1, v2},
 *          Face4 = {v1, v2, v3},
 *
 * Solver Data:
 *
 *  Geometry matrix: [6 x 12] (Shape function gradient)
 *      Used in computing Stiffness Matrix
 *
 *  Force vector: [12x1]
 *      Body force (e.g. gravity) (f)
 *      Traction force (e.g. collision) (t)
 *
 * NOTE:
 *  1) Special care must be taken for Local vertex ordering (TetrahedronSolver does NOT do any ordering)
 *      Swap local ordering of vertices
 *      Pick a vertex as initial one 1.
 *      Pick a face containing the first three vertices 1,2,3 (including the initial one).
 *      The vertex 4 is the one opposite of chosen face.
 *      Number the first three vertices in counterclockwise wise order as seen from vertex 4
 *
 *  2) Research Materials:
 *      http://www.colorado.edu/engineering/CAS/courses.d/AFEM.d/AFEM.Ch09.d/AFEM.Ch09.pdf
 */
template<class T>
class TetrahedronSolver : public FiniteElementSolver<T> {
public:
    TetrahedronSolver() = default;
    ~TetrahedronSolver() = default;

    virtual FiniteElementSolverData<T> Solve(
        std::shared_ptr<FiniteElement<T>> finite_element,
        const std::vector<std::shared_ptr<Vertex<T>>> &vertices,
        std::vector<TriangleFace<T>> &triangle_faces,
        const Eigen::Vector3<T> &body_force,
        const Material<T>& material) override;

    Eigen::Matrix<T,
                  TETRAHEDRON_JACOBIAN_MATRIX_N,
                  TETRAHEDRON_JACOBIAN_MATRIX_N>
    SolveJacobianInverse(std::shared_ptr<FiniteElement<T>> finite_element,
                         const std::vector<std::shared_ptr<Vertex<T>>> &vertices);

private:

    std::vector<TriangleFace<T>*> FetchOrderedTriangleFaces(
        unsigned int vertex_index1,
        unsigned int vertex_index2,
        unsigned int vertex_index3,
        unsigned int vertex_index4,
        TriangleFace<T>& triangle_face1,
        TriangleFace<T>& triangle_face2,
        TriangleFace<T>& triangle_face3,
        TriangleFace<T>& triangle_face4
    );

    /**
     * Fetches triaction forces from each triangle face.
     * The ordering is important and is respected in farther computations.
     * @param triangle_face1
     * @param triangle_face2
     * @param triangle_face3
     * @param triangle_face4
     * @return
     */
    TractionForces<T> FetchTractionForce(
        const std::vector<TriangleFace<T>*>& ordered_triangle_faces);

    Edges<T> ComputeEdgesCache(const Vertex<T> &v1, const Vertex<T> &v2,
                               const Vertex<T> &v3, const Vertex<T> &v4);

    FacesNormal<T> ComputeFacesNormal(const Edges<T> &edges);
    FacesArea<T> ComputeFacesArea(const FacesNormal<T>& face_normals);
    void NormalizeFacesNormal(FacesNormal<T> &normals);
    void SaveTriangleFaceData(
        std::vector<TriangleFace<T>*>& ordered_triangle_faces,
        const FacesArea<T>& faces_area,
        const FacesNormal<T>& faces_normal,
        const TetrahedronShapeFunctionCoefficients<T>& coefficients);

    /**
     * Computes The coefficients on linear tetrahedron shape function.
     * fi(x) = (Ai + Bi*x1 + Ci*x2 + Di*x3) / 6*V
     *
     * @param v1
     * @param v2
     * @param v3
     * @param v4
     * @return
     */
    TetrahedronShapeFunctionCoefficients<T>
    ComputeShapeFunctionCoefficients(const Vertex<T> &v1, const Vertex<T> &v2,
                                     const Vertex<T> &v3, const Vertex<T> &v4,
                                     const Edges<T> &edges);

    Eigen::Vector4<T> ComputeAi(const Vertex<T> &v1, const Vertex<T> &v2,
                                const Vertex<T> &v3, const Vertex<T> &v4);

    T ComputeVolume(const TetrahedronShapeFunctionCoefficients<T> &coefficients);

    /**
     * Computes The [6x12] Geometric Matrix (B)
     *
     * @param coefficients
     * @param volume
     * @return
     */
    Eigen::Matrix<T,
                  TETRAHEDRON_GEOMETRIC_MATRIX_N,
                  TETRAHEDRON_GEOMETRIC_MATRIX_M>
    ComputeGeometryMatrix(const TetrahedronShapeFunctionCoefficients<T> &coefficients,
                          T volume);

    void AssemblyGeometryMatrix(
        Eigen::Matrix<T,
                      TETRAHEDRON_GEOMETRIC_MATRIX_N,
                      TETRAHEDRON_GEOMETRIC_MATRIX_M> &B,
        unsigned int column_offset,
        T b, T c, T d);

    /**
     * Computes Force vector [12x1] which is a combination of body force and traction force.
     *
     * @param shape_function_coefficients
     * @param body_force
     * @param traction_force
     * @return
     */
    Eigen::Vector<T, TETRAHEDRON_FORCE_VECTOR_N>
    ComputeForceVector(const TetrahedronShapeFunctionCoefficients<T> &shape_function_coefficients,
                       T volume, const FacesArea<T> &faces_area,
                       const Eigen::Vector3<T> &body_force,
                       const TractionForces<T> &traction_force,
                       const Material<T>& material);
    Eigen::Vector<T, TETRAHEDRON_FORCE_VECTOR_N>
    ComputeBodyForceVector(T volume,
                           const Eigen::Vector3<T> &body_force,
                           const Material<T>& material);
    Eigen::Vector<T, TETRAHEDRON_FORCE_VECTOR_N>
    ComputeTractionForceVector(const TetrahedronShapeFunctionCoefficients<T> &shape_function_coefficients,
                               const FacesArea<T> &faces_area,
                               const TractionForces<T> &traction_force);
    Eigen::Vector<T, TETRAHEDRON_FORCE_VECTOR_N>
    ComputeTractionForceVectorFace(unsigned int face_index, T traction_force,
                                   T area,
                                   T B, T C, T D);

    Eigen::Matrix<T,
                  TETRAHEDRON_JACOBIAN_MATRIX_N,
                  TETRAHEDRON_JACOBIAN_MATRIX_N>
    AssemblyJacobianInverse(const TetrahedronShapeFunctionCoefficients<T> &coefficients,
                            T volume);
};
}

#endif //PROJECT_TETRAHEDRONSOLVER_H
