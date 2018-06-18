#ifndef LIMITSMT_H
#define LIMITSMT_H

#include "expr/expression.h"
#include "its/variablemanager.h"
#include "limitproblem.h"

namespace LimitSmtEncoding {
    /**
     * Checks whether the SMT encoding is applicable to the limit problem.
     * This only depends on the costs (they have to be polynomial for the encoding).
     */
    bool isApplicable(const Expression &cost);

    /**
     * Tries to solve the given limit problem by an encoding into a SMT query.
     * @returns the found solution (if any), the limit problem is not modified.
     */
    option<GiNaC::exmap> applyEncoding(const LimitProblem &currentLP, const Expression &cost,
                                       const VarMan &varMan, bool finalCheck);
}

#endif //LIMITSMT_H
