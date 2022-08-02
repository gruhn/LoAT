/*  This file is part of LoAT.
 *  Copyright (c) 2015-2016 Matthias Naaf, RWTH Aachen University, Germany
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses>.
 */

#include "preprocess.hpp"

#include "../expr/guardtoolbox.hpp"
#include "../smt/smt.hpp"
#include "../smt/z3/z3.hpp"
#include "../smt/smtfactory.hpp"

using namespace std;


Result<Rule> Preprocess::preprocessRule(ITSProblem &its, const Rule &rule) {
    Result<Rule> res(rule);

    // The other steps are repeated (might not help very often, but is probably cheap enough)
    bool changed = false;
    do {
        changed = false;
        Result<Rule> tmp = eliminateTempVars(its, *res, true);
        changed = changed || tmp;
        res.concat(tmp);
        tmp = removeTrivialUpdates(*res, its);
        changed = changed || tmp;
        res.concat(tmp);
    } while (changed);

    res.concat(simplifyGuard(*res, its));
    return res;
}

Result<Rule> Preprocess::simplifyRule(ITSProblem &its, const Rule &rule, bool fast) {
    Result<Rule> res(rule);
    res.concat(eliminateTempVars(its, *res, fast));
    res.concat(simplifyGuard(*res, its));
    res.concat(removeTrivialUpdates(*res, its));
    return res;
}


Result<Rule> Preprocess::simplifyGuard(const Rule &rule, const ITSProblem &its) {
    Result<Rule> res(rule);
    const BoolExpr newGuard = Z3::simplify(rule.getGuard(), its);
    if (rule.getGuard() != newGuard) {
        const Rule newRule = rule.withGuard(newGuard);
        res.set(newRule);
        res.ruleTransformationProof(rule, "simplified guard with Z3", newRule, its);
    }
    return res;
}


Result<Rule> Preprocess::removeTrivialUpdates(const Rule &rule, const ITSProblem &its) {
    bool changed = false;
    std::vector<RuleRhs> newRhss;
    for (const RuleRhs &rhs: rule.getRhss()) {
        Subs up = rhs.getUpdate();
        changed |= removeTrivialUpdates(up);
        newRhss.push_back(RuleRhs(rhs.getLoc(), up));
    }
    Result<Rule> res{Rule(rule.getLhs(), newRhss), changed};
    if (res) {
        res.ruleTransformationProof(rule, "removed trivial updates", res.get(), its);
    }
    return res;
}

bool Preprocess::removeTrivialUpdates(Subs &update) {
    stack<Var> remove;
    for (auto it : update) {
        if (it.second.equals(it.first)) {
            remove.push(it.first);
        }
    }
    if (remove.empty()) return false;
    while (!remove.empty()) {
        update.erase(remove.top());
        remove.pop();
    }
    return true;
}


/**
 * Returns the set of all variables that appear in the rhs of some update.
 * For an update x := a and x := x+a, this is {a} and {x,a}, respectively.
 */
static VarSet collectVarsInUpdateRhs(const Rule &rule) {
    VarSet varsInUpdate;
    for (auto rhs = rule.rhsBegin(); rhs != rule.rhsEnd(); ++rhs) {
        for (const auto &it : rhs->getUpdate()) {
            it.second.collectVars(varsInUpdate);
        }
    }
    return varsInUpdate;
}


Result<Rule> Preprocess::eliminateTempVars(ITSProblem &its, const Rule &rule, bool fast) {
    Result<Rule> res(rule);

    //declare helper lambdas to filter variables, to be passed as arguments
    auto isTemp = [&](const Var &sym) {
        return its.isTempVar(sym);
    };
    auto isTempInUpdate = [&](const Var &sym) {
        VarSet varsInUpdate = collectVarsInUpdateRhs(*res);
        return isTemp(sym) && varsInUpdate.count(sym) > 0;
    };
    auto isTempOnlyInGuard = [&](const Var &sym) {
        VarSet varsInUpdate = collectVarsInUpdateRhs(*res);
        return isTemp(sym) && varsInUpdate.count(sym) == 0 && !rule.getCost().has(sym);
    };

    //equalities allow easy propagation, thus transform x <= y, x >= y into x == y
    res.concat(GuardToolbox::makeEqualities(*res, its));
    res.fail(); // *just* finding implied equalities does not suffice for success

    //try to remove temp variables from the update by equality propagation (they are removed from guard and update)
    res.concat(GuardToolbox::propagateEqualities(its, *res, GuardToolbox::ResultMapsToInt, isTempInUpdate));

    //try to remove all remaining temp variables (we do 2 steps to priorizie removing vars from the update)
    res.concat(GuardToolbox::propagateEqualities(its, *res, GuardToolbox::ResultMapsToInt, isTemp));

    if (!fast && !res->getGuard()->isConjunction()) {
        res.concat(GuardToolbox::propagateEqualitiesBySmt(*res, its));
    }

    option<BoolExpr> newGuard = res->getGuard()->simplify();
    if (newGuard) {
        const Rule newRule = res->withGuard(newGuard.get());
        res.ruleTransformationProof(res.get(), "simplified guard", newRule, its);
        res = newRule;
    }

    //now eliminate a <= x and replace a <= x, x <= b by a <= b for all free variables x where this is sound
    //(not sound if x appears in update or cost, since we then need the value of x)
    res.concat(GuardToolbox::eliminateByTransitiveClosure(*res, true, isTempOnlyInGuard, its));

    return res;
}
