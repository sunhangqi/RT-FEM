#ifndef PROJECT_FEMDYNAMICSOLVER_H
#define PROJECT_FEMDYNAMICSOLVER_H

#include <RTFEM/FEM/Solver/FEMSolver.h>
#include <RTFEM/FEM/Solver/FEMGlobalAssembler.h>
#include "RTFEM/GPU/LinearSolver/GPULinearSolver.cuh"
#include "RTFEM/GPU/GPUMVMultiplication.cuh"

namespace rtfem {

/**
 * http://www.sciencedirect.com/science/article/pii/S0045794915001479
 * @tparam T
 */
template<class T>
class FEMDynamicSolver : public FEMSolver<T>{
public:

    FEMDynamicSolver(FEMModel<T>* fem_model);
    ~FEMDynamicSolver() = default;

    const FEMSolverOutput<T>* solver_output(){return &solver_output_;}

    T total_time(){return total_time_;}

    const FEMSolverTimer& timer(){return timer_;}

    virtual FEMSolverOutput<T> Solve() override;

    void RunIteration(T delta_time);

private:
    void ReassembleForces();
    void SolveForDisplacements(T delta_time);
    void ResetForces();

    void ImplicitNewtonGPU(T delta_time);
    void SolveRHSGPU(T delta_time,
                     Eigen::Vector<T, Eigen::Dynamic>& global_force);
    void SolveBoundaryConditions(Eigen::Vector<T, Eigen::Dynamic>& rhs);
    void SolveLinearSystem(const Eigen::Vector<T, Eigen::Dynamic>& rhs,
                           Eigen::Vector<T, Eigen::Dynamic>& velocity);
    void SolveIntegration(
        T delta_time,
        const Eigen::Vector<T, Eigen::Dynamic>& new_velocity);

    void ImplicitNewtonCPU(T delta_time);

    GPULinearSolver<T> gpu_linear_solver_;

    GPUMVMultiplication<T> gpu_multiplication_rhs_mass_;
    GPUMVMultiplication<T> gpu_multiplication_rhs_stiffness_;

    FEMSolverOutput<T> solver_output_;

    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> left_hand_side_;
    Eigen::Vector<T, Eigen::Dynamic> displacement_velocity_current_;
    Eigen::Vector<T, Eigen::Dynamic> displacement_acceleration_current_;
    FEMGlobalAssemblerData<T> fem_assembler_data_;

    T total_time_;

    FEMSolverTimer timer_;
};
}


#endif //PROJECT_FEMDYNAMICSOLVER_H
