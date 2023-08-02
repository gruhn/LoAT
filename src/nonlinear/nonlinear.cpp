#include "nonlinear.hpp"
#include "booltheory.hpp"
#include <cassert>

const std::vector<Var> renameWith(const std::vector<Var> input, const Subs renaming) {
    std::vector<Var> output;
    output.reserve(input.size());
  
    for (const Var &var: input) {
        if (std::holds_alternative<NumVar>(var)) {
            const NumVar num_var = std::get<NumVar>(var);
            const Expr num_expr = renaming.get<IntTheory>().get(num_var);

            assert(num_expr.isVar());
            output.push_back(num_expr.toVar());
        } else if (std::holds_alternative<BoolVar>(var)) {
            const BoolVar bool_var = std::get<BoolVar>(var);
            const BoolExpr bool_expr = renaming.get<BoolTheory>().get(bool_var); 

            // TODO: there's probably a simpler way to restore a BoolVar from a BoolExpr?
            const auto bool_expr_vars = bool_expr->vars();
            assert(bool_expr_vars.size() == 1);
            const BoolVar target_var = std::get<BoolVar>(*bool_expr_vars.begin());

            output.push_back(target_var);
        } else {
            throw std::logic_error("unsupported theory in NonLinearSolver");
        }
    }

    return output;
}

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

  const std::vector<Var> lhs_args = progVarsOrdered;
  const std::vector<Var> rhs_args = renameWith(progVarsOrdered, rule.getUpdate());;

  const auto rhs = FunApp(rhs_loc, rhs_args);

  if (lhs_loc == its.getInitialLocation()) {     
    // rule is a linear CHC with no LHS predicates, ie a "fact"
    return Clause({}, rhs, guard);
  } else {
    // rule is a linear CHC with exactly one LHS predicates, ie a "rule"
    return Clause({ FunApp(lhs_loc, lhs_args) }, rhs, guard);  
  }
};

void NonLinearSolver::analyze(ITSProblem &its) {
  const auto solver = NonLinearSolver(its);

  // std::cout << "Non Linear CHCs: " << std::endl;
  // for (const Clause &c: its.nonLinearCHCs) {
  //     std::cout << c << std::endl;
  // }

  std::cout << "Linear CHCs (restored): " << std::endl;
  for (const auto trans_idx : its.getAllTransitions()) {
    auto c = solver.clauseFrom(trans_idx);
    std::cout << c << std::endl;
  }

  // std::cout << "Chaining: " << std::endl;
  // for (const auto trans_idx1 : its.getAllTransitions()) {
  //   if (its.getLhsLoc(trans_idx1) == its.getInitialLocation()) {
  //     continue;
  //   }

  //   for (const auto trans_idx2 : its.getAllTransitions()) {
  //     if (its.getLhsLoc(trans_idx2) == its.getInitialLocation()) {
  //       continue;
  //     }

  //     if (trans_idx1 != trans_idx2) {
  //       const Rule rule1 = its.getRule(trans_idx1);
  //       const Rule rule2 = its.getRule(trans_idx2);
  //       const Rule chained = rule1.chain(rule2);

  //       std::cout << "Rule 1:  " << rule1 << std::endl;
  //       std::cout << "Rule 2:  " << rule2 << std::endl;
  //       std::cout << "Chained: " << chained << std::endl;       
  //     }
  //   }
  // }
}

// const std::optional<Clause> NonLinearSolver::resolution(TransIdx linear_chc,
// Clause &chc) const {
//   const LocationIdx linear_chc_lhs = its.getLhsLoc(linear_chc);
//   const LocationIdx linear_chc_rhs = its.getRhsLoc(linear_chc);

//   const auto extraction = extractFromLhs(chc, linear_chc_rhs);

//   if (extraction) {
//     const auto [fun, chc_without_fun] = extraction.value();

//   } else {
//     return {};
//   }
// }
