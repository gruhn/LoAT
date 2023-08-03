#include "nonlinear.hpp"
#include "booltheory.hpp"
#include <cassert>

bool isLocVar(const Var &var) {
    if (std::holds_alternative<NumVar>(var)) {
        return std::get<NumVar>(var) == NumVar::loc_var;
    } else {
        return false;
    }
}

const std::vector<Var> getProgVarsOrdered(const ITSProblem &its) {
  std::vector<Var> args;

  for (const Var &var : its.getVars()) {
    if (expr::isProgVar(var) && !isLocVar(var)) {
      args.push_back(var);
    }
  }

  return args;
}

NonLinearSolver::NonLinearSolver(const ITSProblem &its)
    : its(its), progVarsOrdered(getProgVarsOrdered(its)) {}

/** 
 * Converts an ITS rule (identified by a TransIdx) back to CHC representation.
 * This does not restore the original representation after parsing perfectly,
 * since number and order of predicate arguments is lost.
 */
const Clause NonLinearSolver::clauseFrom(TransIdx trans_idx) const {
  const Rule rule = its.getRule(trans_idx);
  const auto guard = rule.getGuard();

  const LocationIdx lhs_loc = its.getLhsLoc(trans_idx);
  const LocationIdx rhs_loc = its.getRhsLoc(trans_idx);

  const auto rhs = renameWith(FunApp(rhs_loc, progVarsOrdered), rule.getUpdate());

  if (lhs_loc == its.getInitialLocation()) {     
    // rule is a linear CHC with no LHS predicates, ie a "fact"
    return Clause({}, rhs, guard);
  } else {
    // rule is a linear CHC with exactly one LHS predicates, ie a "rule"
    return Clause({ FunApp(lhs_loc, progVarsOrdered) }, rhs, guard);  
  }
};

void NonLinearSolver::analyze(ITSProblem &its) {
  const auto solver = NonLinearSolver(its);

  std::cout << "=== Resolution Test ===" << std::endl;
  for (const auto trans_idx : its.getAllTransitions()) {
     const Clause linear_chc = solver.clauseFrom(trans_idx);

     for (const Clause &non_linear_chc: its.nonLinearCHCs) {
         for (const auto &pred: non_linear_chc.lhs) {
             const auto resolvent = resolutionWith(linear_chc, non_linear_chc, pred);       

             if (resolvent.has_value()) {
                 std::cout << " -------------------------------------------- " << std::endl;
                 std::cout << "Linear CHC     : " << linear_chc                << std::endl;
                 std::cout << "Non Linear CHC : " << non_linear_chc            << std::endl;
                 std::cout << "Predicate      : " << pred                      << std::endl;
                 std::cout << "Resolvent      : " << resolvent.value()         << std::endl;
                 std::cout << " -------------------------------------------- " << std::endl;
             }
          }
     }
  }
}
