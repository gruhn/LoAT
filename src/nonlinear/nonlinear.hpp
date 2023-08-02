#pragma once

#include "itsproblem.hpp"

class NonLinearSolver {

private:

    NonLinearSolver(const ITSProblem &its);

    ITSProblem its;

    // To restore a `Clause` from an ITS transition, we need to choose an order for
    // the predicate arguments. In ITS transitions the order is arbitrary, so to 
    // pick an order consistently, we fix order here.
    const std::vector<Var> progVarsOrdered;
    
public:

    static void analyze(ITSProblem &its);

    const Clause clauseFrom(TransIdx trans_idx) const;

};
