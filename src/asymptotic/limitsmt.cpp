#include "limitsmt.h"

#include "z3/z3solver.h"
#include "z3/z3context.h"
#include "z3/z3toolbox.h"

#include "inftyexpression.h"
#include "config.h"
#include "util/timeout.h"

using namespace std;


/**
 * Given the (abstract) coefficients of a univariate polynomial p in n (where the key is the
 * degree of the respective monomial), builds an expression which implies that
 * lim_{n->\infty} p is a positive constant
 */
static z3::expr posConstraint(const map<int, Expression>& coefficients, Z3Context& context) {
    z3::expr conjunction = context.bool_val(true);
    for (pair<int, Expression> p : coefficients) {
        int degree = p.first;
        Expression c = p.second;
        if (degree > 0) {
            conjunction = conjunction && c.toZ3(context) == 0;
        } else {
            conjunction = conjunction && c.toZ3(context) > 0;
        }
    }
    return conjunction;
}

/**
 * Given the (abstract) coefficients of a univariate polynomial p in n (where the key is the
 * degree of the respective monomial), builds an expression which implies that
 * lim_{n->\infty} p is a negative constant
 */
static z3::expr negConstraint(const map<int, Expression>& coefficients, Z3Context& context) {
    z3::expr conjunction = context.bool_val(true);
    for (pair<int, Expression> p : coefficients) {
        int degree = p.first;
        Expression c = p.second;
        if (degree > 0) {
            conjunction = conjunction && c.toZ3(context) == 0;
        } else {
            conjunction = conjunction && c.toZ3(context) < 0;
        }
    }
    return conjunction;
}

/**
 * Given the (abstract) coefficients of a univariate polynomial p in n (where the key is the
 * degree of the respective monomial), builds an expression which implies
 * lim_{n->\infty} p = -\infty
 */
static z3::expr negInfConstraint(const map<int, Expression>& coefficients, Z3Context& context) {
    int maxDegree = 0;
    for (pair<int, Expression> p: coefficients) {
        maxDegree = p.first > maxDegree ? p.first : maxDegree;
    }
    z3::expr disjunction = context.bool_val(false);
    for (int i = 1; i <= maxDegree; i++) {
        z3::expr conjunction = context.bool_val(true);
        for (pair<int, Expression> p: coefficients) {
            int degree = p.first;
            Expression c = p.second;
            if (degree > i) {
                conjunction = conjunction && c.toZ3(context) == 0;
            } else if (degree == i) {
                conjunction = conjunction && c.toZ3(context) < 0;
            }
        }
        disjunction = disjunction || conjunction;
    }
    return disjunction;
}

/**
 * Given the (abstract) coefficients of a univariate polynomial p in n (where the key is the
 * degree of the respective monomial), builds an expression which implies
 * lim_{n->\infty} p = \infty
 */
static z3::expr posInfConstraint(const map<int, Expression>& coefficients, Z3Context& context) {
    int maxDegree = 0;
    for (pair<int, Expression> p: coefficients) {
        maxDegree = p.first > maxDegree ? p.first : maxDegree;
    }
    z3::expr disjunction = context.bool_val(false);
    for (int i = 1; i <= maxDegree; i++) {
        z3::expr conjunction = context.bool_val(true);
        for (pair<int, Expression> p: coefficients) {
            int degree = p.first;
            Expression c = p.second;
            if (degree > i) {
                conjunction = conjunction && c.toZ3(context) == 0;
            } else if (degree == i) {
                conjunction = conjunction && c.toZ3(context) > 0;
            }
        }
        disjunction = disjunction || conjunction;
    }
    return disjunction;
}

/**
 * @return the (abstract) coefficients of 'n' in 'ex', where the key is the degree of the respective monomial
 */
static map<int, Expression> getCoefficients(Expression ex, ExprSymbol n) {
    // TODO: This seems overly complicated
    GiNaC::lst nList;
    nList.append(n);
    int maxDegree = ex.getMaxDegree(nList);
    map<int, Expression> coefficients;
    for (int i = 0; i <= maxDegree; i++) {
        coefficients.emplace(i, ex.coeff(n, i));
    }
    return coefficients;
}

static bool isTimeout(bool finalCheck) {
    return finalCheck ? Timeout::hard() : Timeout::soft();
}


bool LimitSmtEncoding::isApplicable(const Expression &cost) {
    return cost.isPolynomial();
}


option<GiNaC::exmap> LimitSmtEncoding::applyEncoding(const LimitProblem &currentLP, const Expression &cost,
                                                     const VarMan &varMan, bool finalCheck)
{
    debugAsymptoticBound(endl << "SMT: " << currentLP << endl);

    // initialize z3
    Z3Context context;
    Z3Solver solver(context, Config::Z3::LimitTimeout);

    // the parameter of the desired family of solutions
    ExprSymbol n = currentLP.getN();

    // get all relevant variables
    ExprSymbolSet vars = currentLP.getVariables();

    // create linear templates for all variables
    GiNaC::exmap templateSubs;
    map<ExprSymbol,z3::expr,GiNaC::ex_is_less> varCoeff, varCoeff0;
    for (const ExprSymbol &var : vars) {
        ExprSymbol c0 = varMan.getFreshUntrackedSymbol(var.get_name()+"_0");
        ExprSymbol c = varMan.getFreshUntrackedSymbol(var.get_name()+"_c");
        varCoeff.emplace(var,GinacToZ3::convert(c,context)); //HACKy HACK  // FIXME: HACKy HACK sounds dangerous...
        varCoeff0.emplace(var,GinacToZ3::convert(c0,context));
        templateSubs[var] = c0 + (n * c);
    }

    // replace variables in the cost function with their linear templates
    assert(cost.isPolynomial()); // as checked in isApplicable()
    Expression templateCost = cost.subs(templateSubs).expand();

    // if the cost function is a constant, then we are bound to fail
    // TODO: This seems overly complicated
    GiNaC::lst nList;
    nList.append(n);
    int maxDeg = templateCost.getMaxDegree(nList);
    if (maxDeg == 0) {
        return {};
    }

    // encode every entry of the limit problem
    for (auto it = currentLP.cbegin(); it != currentLP.cend(); ++it) {
        // replace variables with their linear templates
        Expression ex = (*it).subs(templateSubs).expand();
        map<int, Expression> coefficients = getCoefficients(ex, n);
        Direction direction = it->getDirection();
        // add the required constraints (depending on the direction-label from the limit problem)
        if (direction == POS) {
            z3::expr disjunction = posConstraint(coefficients, context) || posInfConstraint(coefficients, context);
            solver.add(disjunction);
        } else if (direction == POS_CONS) {
            solver.add(posConstraint(coefficients, context));
        } else if (direction == POS_INF) {
            solver.add(posInfConstraint(coefficients, context));
        } else if (direction == NEG_CONS) {
            solver.add(negConstraint(coefficients, context));
        } else if (direction == NEG_INF) {
            solver.add(negInfConstraint(coefficients, context));
        }
    }

    // auxiliary function that checks satisfiability wrt. the current state of the solver
    auto checkSolver = [&]() -> bool {
        debugAsymptoticBound("SMT Query: " << solver);
        z3::check_result res = solver.check();
        debugAsymptoticBound("SMT Result: " << res);
        if (res == z3::sat) {
            return true;
        } else {
            return false;
        }
    };

    // all constraints that we have so far are mandatory, so fail if we can't even solve these
    if (!checkSolver()) {
        return {};
    }

    // remember the current state for backtracking before trying several variations
    solver.push();

    // first fix that all program variables have to be constants
    // a model witnesses unbounded complexity
    for (const ExprSymbol &var : vars) {
        if (!varMan.isTempVar(var)) {
            solver.add(varCoeff.at(var) == 0);
        }
    }

    if (!checkSolver()) {
        // we failed to find a model -- drop all non-mandatory constraints
        solver.pop();
        // try to find a witness for polynomial complexity with degree maxDeg,...,1
        map<int, Expression> coefficients = getCoefficients(templateCost, n);
        for (int i = maxDeg; i > 0; i--) {
            if (isTimeout(finalCheck)) return {};
            Expression c = coefficients.find(i)->second;
            // remember the current state for backtracking
            solver.push();
            solver.add(c.toZ3(context) > 0);
            if (checkSolver()) {
                break;
            } else if (i == 1) {
                // we even failed to prove a linear bound -- give up
                return {};
            } else {
                // remove all non-mandatory constraints and retry with degree i-1
                solver.pop();
            }
        }
    }

    debugAsymptoticBound("SMT Model: " << solver.get_model() << endl);

    // we found a model -- create the corresponding solution of the limit problem
    GiNaC::exmap smtSubs;
    z3::model model = solver.get_model();
    for (const ExprSymbol &var : vars) {
        Expression c0 = Z3Toolbox::getRealFromModel(model,varCoeff0.at(var));
        Expression c = Z3Toolbox::getRealFromModel(model,varCoeff.at(var));
        smtSubs[var] = c0 + c * n;
    }

    return smtSubs;
}

