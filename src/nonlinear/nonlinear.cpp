#include "nonlinear.hpp"
#include "booltheory.hpp"
#include "linearsolver.hpp"
#include <stdexcept>

void NonLinearSolver::analyze(ILinearSolver &linear_solver) {
    std::stack<Clause, std::list<Clause>> facts(linear_solver.get_facts());    

    while (true) {
        // std::cout << "================" << std::endl;
        std::list<Clause> resolvents;

        // Do resolution with of all `facts` with all non-linear clauses with the goal to derive
        // new linear clauses that the linear solver can handle.        
        while (!facts.empty()) {
            const Clause fact = facts.top();
            facts.pop();

            // Note, we don't add resolvents to the ITS during this loop but instead collect on a stack,
            // and then add the contained clauses to the ITS afterwards in a dedicated loop. 
            // Otherwise, the following for-loop is "dynamically extended" because we also loop 
            // over the newly added clauses.
            for (const Clause &non_linear_chc: linear_solver.get_non_linear_chcs()) {
                for (const auto &pred: non_linear_chc.lhs) {
                    const auto optional_resolvent = fact.resolutionWith(non_linear_chc, pred);

                    if (optional_resolvent.has_value()) {
                        const auto resolvent = optional_resolvent.value();
                        // TODO: check for redundancy
                        resolvents.push_back(resolvent);
                    }
                }
            }
        }

        linear_solver.add_clauses(resolvents);
       
        // ...
        while (true) {
            const auto new_fact = linear_solver.derive_new_fact();
            // TOOD: why do I get redundant facts here?
            if (new_fact.has_value()) {
                // std::cout << "New Fact: " << new_fact.value() << std::endl;
                facts.push(new_fact.value());
            } else {
                break;
            }
        }

        const auto result = linear_solver.get_analysis_result();     
        if (result == LinearSolver::Result::Unsat) {
            break;
        } else if ((result == LinearSolver::Result::Sat || result == LinearSolver::Result::Unknown) && facts.empty()) {
            break;
        }
    } 
}
