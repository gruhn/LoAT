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

#include "analysis.hpp"

#include "../expr/relation.hpp"
#include "../smt/smt.hpp"
#include "../asymptotic/asymptoticbound.hpp"

#include "../util/timeout.hpp"

#include "prune.hpp"
#include "preprocess.hpp"
#include "chain.hpp"
#include "chainstrategy.hpp"
#include "../accelerate/accelerator.hpp"
#include "../its/export.hpp"

#include <future>

#ifdef HAS_YICES
    #include <yices.h>
#endif

using namespace std;



void Analysis::analyze(ITSProblem &its) {
    Analysis analysis(its);
    analysis.run();
}


Analysis::Analysis(ITSProblem &its)
        : its(its) {}


// ##############################
// ## Main Analysis Algorithm  ##
// ##############################

void Analysis::simplify(RuntimeResult &res) {

    ProofOutput::Proof.majorProofStep("Initial ITS", its);

    if (Config::Analysis::EnsureNonnegativeCosts && ensureNonnegativeCosts()) {
        ProofOutput::Proof.minorProofStep("Ensure Cost >= 0", its);
    }

    if (ensureProperInitialLocation()) {
        ProofOutput::Proof.minorProofStep("Added a fresh start location without incoming rules", its);
    }

    string eliminatedLocation; // for proof output of eliminateALocation
    bool acceleratedOnce = false; // whether we did at least one acceleration step
    bool nonlinearProblem = !its.isLinear(); // whether the ITS is (still) nonlinear

    // Check if we have at least constant complexity (i.e., at least one rule can be taken with cost >= 1)
    if (Config::Analysis::ConstantCpxCheck) {
        checkConstantComplexity(res);
    }

    if (Config::Analysis::Preprocessing) {

        if (Pruning::removeLeafsAndUnreachable(its)) {
            ProofOutput::Proof.minorProofStep("Removed unreachable rules and leafs", its);
        }

        if (removeUnsatRules()) {
            ProofOutput::Proof.minorProofStep("Removed rules with unsatisfiable guard", its);
        }

        if (Pruning::removeLeafsAndUnreachable(its)) {
            ProofOutput::Proof.minorProofStep("Removed unreachable rules and leafs", its);
        }

        if (preprocessRules()) {
            ProofOutput::Proof.minorProofStep("Simplified rules", its);
        }

    }

    // We cannot prove any lower bound for an empty ITS
    if (its.isEmpty()) {
        return;
    }

    while (!isFullySimplified()) {

        // Repeat linear chaining and simple loop acceleration
        bool changed;
        do {
            changed = false;
            set<TransIdx> acceleratedRules;

            // Special handling of nonlinear rules
            if (nonlinearProblem && Pruning::removeSinkRhss(its)) {
                changed = true;
                ProofOutput::Proof.majorProofStep("Removed sinks", its);
            }

            if (accelerateSimpleLoops(acceleratedRules)) {
                changed = true;
                acceleratedOnce = true;
                ProofOutput::Proof.majorProofStep("Accelerated simple loops", its);
            }

            if (chainAcceleratedLoops(acceleratedRules)) {
                changed = true;
                ProofOutput::Proof.majorProofStep("Chained accelerated rules with incoming rules", its);
            }

            if (Pruning::removeLeafsAndUnreachable(its)) {
                changed = true;
                ProofOutput::Proof.majorProofStep("Removed unreachable locations and irrelevant leafs", its);
            }

            if (chainLinearPaths()) {
                changed = true;
                ProofOutput::Proof.majorProofStep("Eliminated locations on linear paths", its);
            }

            // Check if the ITS is now linear (we accelerated all nonlinear rules)
            if (changed && nonlinearProblem) {
                nonlinearProblem = !its.isLinear();
            }
        } while (changed);

        // Avoid wasting time on chaining/pruning if we are already done
        if (isFullySimplified()) {
            break;
        }

        // Try more involved chaining strategies if we no longer make progress
        if (chainTreePaths()) {
            ProofOutput::Proof.majorProofStep("Eliminated locations on tree-shaped paths", its);

        } else if (eliminateALocation(eliminatedLocation)) {
            ProofOutput::Proof.majorProofStep("Eliminated location " + eliminatedLocation, its);
        }
        if (isFullySimplified()) break;

        if (acceleratedOnce) {

            // Try to avoid rule explosion (often caused by chainTreePaths).
            // Since pruning relies on the rule's complexities, we only do this after the first acceleration.
            if (pruneRules()) {
                ProofOutput::Proof.majorProofStep("Applied pruning (of leafs and parallel rules):", its);
            }
        }

    }
}

void Analysis::finalize(RuntimeResult &res) {
    its.lock();
    if (!Timeout::soft()) {
        // Remove duplicate rules (ignoring updates) to avoid wasting time on asymptotic bounds
        if (Pruning::removeDuplicateRules(its, its.getTransitionsFrom(its.getInitialLocation()), false)) {
            ProofOutput::Proof.majorProofStep("Removed duplicate rules (ignoring updates)", its);
        }
    }

    if (Config::Output::ExportSimplified) {
        cout << "Fully simplified program in input format:" << endl;
        ITSExport::printKoAT(its, cout);
    }

    ProofOutput::Proof.headline("Computing asymptotic complexity");

    if (Timeout::soft()) {
        // A timeout occurred before we managed to complete the analysis.
        // We try to quickly extract at least some complexity results.
        // Reduce the number of rules to avoid z3 invocations
        removeConstantPathsAfterTimeout();
        // Try to find a high complexity in the remaining problem (with chaining, but without acceleration)
        getMaxPartialResult(res);
    } else {
        // No timeout, fully simplified, find the maximum runtime
        getMaxRuntime(res);
    }
}

void Analysis::run() {
#ifdef HAS_YICES
    yices_init();
#endif

    RuntimeResult *res = new RuntimeResult();
    auto simp = std::async([this, res]{this->simplify(*res);});
    if (Timeout::enabled()) {
        if (simp.wait_for(std::chrono::seconds(Timeout::remainingSoft())) == std::future_status::timeout) {
            std::cerr << "Aborted simplification due to soft timeout" << std::endl;
        }
    } else {
        simp.wait();
    }
    auto finalize = std::async([this, res]{this->finalize(*res);});
    if (Timeout::enabled()) {
        long remaining = Timeout::remainingHard();
        if (remaining > 0) {
            if (finalize.wait_for(std::chrono::seconds(remaining)) == std::future_status::timeout) {
                std::cerr << "Aborted analysis of simplified ITS due to timeout" << std::endl;
            }
        }
    } else {
        finalize.wait();
    }
    res->mutex.lock();
    printResult(*res);
    // WST style proof output
    cout << res->cpx.toWstString() << std::endl;
    ProofOutput::Proof.print();
    res->mutex.unlock();

    delete res;
#ifdef HAS_YICES
    yices_exit();
#endif
    if (simp.wait_for(std::chrono::seconds(0)) != std::future_status::ready || finalize.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        std::cerr << "some tasks are still running, calling std::terminte" << std::endl;
        std::terminate();
    }
}

// ############################
// ## Preprocessing, Output  ##
// ############################

bool Analysis::ensureProperInitialLocation() {
    if (its.hasTransitionsTo(its.getInitialLocation())) {
        LocationIdx newStart = its.addLocation();
        its.addRule(Rule::dummyRule(newStart, its.getInitialLocation()));
        its.setInitialLocation(newStart);
        return true;
    }
    return false;
}


bool Analysis::ensureNonnegativeCosts() {
    bool changed = false;

    for (TransIdx trans : its.getAllTransitions()) {
        its.try_lock();
        Rule &rule = its.getRuleMut(trans);

        // Add the constraint unless it is trivial (e.g. if the cost is 1).
        Expression costConstraint = rule.getCost() >= 0;
        if (!Relation::isTriviallyTrue(costConstraint)) {
            rule.getGuardMut().push_back(costConstraint);
            changed = true;
        }
        its.unlock();
    }
    return changed;
}


bool Analysis::removeUnsatRules() {
    bool changed = false;

    for (TransIdx rule : its.getAllTransitions()) {
        if (Smt::check(buildAnd(its.getRule(rule).getGuard()), its) == Smt::Unsat) {
            its.removeRule(rule);
            changed = true;
        }
    }

    return changed;
}


bool Analysis::preprocessRules() {
    bool changed = false;

    // update/guard preprocessing
    for (TransIdx idx : its.getAllTransitions()) {
        its.try_lock();
        Rule &rule = its.getRuleMut(idx);
        changed = Preprocess::preprocessRule(its, rule) || changed;
        its.unlock();
    }

    // remove duplicates
    for (LocationIdx node : its.getLocations()) {
        for (LocationIdx succ : its.getSuccessorLocations(node)) {
            changed = Pruning::removeDuplicateRules(its, its.getTransitionsFromTo(node, succ)) || changed;
        }
    }

    return changed;
}


bool Analysis::isFullySimplified() const {
    for (LocationIdx node : its.getLocations()) {
        if (its.isInitialLocation(node)) continue;
        if (its.hasTransitionsFrom(node)) return false;
    }
    return true;
}


void Analysis::printResult(const RuntimeResult &runtime) {
    ProofOutput::Proof.newline();
    ProofOutput::Proof.result("Proved the following lower bound");

    ProofOutput::Proof.result(stringstream() << "Complexity:  " << runtime.cpx);
    stringstream s;
    s << "Cpx degree: ";
    switch (runtime.cpx.getType()) {
        case Complexity::CpxPolynomial: s << runtime.cpx.getPolynomialDegree().toFloat() << endl; break;
        case Complexity::CpxUnknown: s << "?" << endl; break;
        default: s << runtime.cpx << endl;
    }
    s << endl;
    s << "Solved cost: " << runtime.solvedCost << endl;
    s << "Rule cost:   ";
    ITSExport::printCost(runtime.cost, s);
    s << endl;
    s << "Rule guard:  ";
    ITSExport::printGuard(runtime.guard, s);
    ProofOutput::Proof.result(s);
}


// ##############################
// ## Acceleration & Chaining  ##
// ##############################

bool Analysis::chainLinearPaths() {
    return Chaining::chainLinearPaths(its);
}


bool Analysis::chainTreePaths() {
    return Chaining::chainTreePaths(its);
}


bool Analysis::eliminateALocation(string &eliminatedLocation) {
    return Chaining::eliminateALocation(its, eliminatedLocation);
}


bool Analysis::chainAcceleratedLoops(const set<TransIdx> &acceleratedRules) {
    return Chaining::chainAcceleratedRules(its, acceleratedRules);
}


bool Analysis::accelerateSimpleLoops(set<TransIdx> &acceleratedRules) {
    bool res = false;

    for (LocationIdx node : its.getLocations()) {
        res = Accelerator::accelerateSimpleLoops(its, node, acceleratedRules) || res;
    }

    return res;
}


bool Analysis::pruneRules() {
    // Always remove unreachable rules
    bool changed = Pruning::removeLeafsAndUnreachable(its);

    // Prune parallel transitions if enabled
    if (Config::Analysis::Pruning) {
        changed = Pruning::pruneParallelRules(its) || changed;
    }

    return changed;
}


// #############################
// ## Complexity Computation  ##
// #############################


void Analysis::checkConstantComplexity(RuntimeResult &res) const {

    for (TransIdx idx : its.getTransitionsFrom(its.getInitialLocation())) {
        const Rule &rule = its.getRule(idx);
        GuardList guard = rule.getGuard();
        guard.push_back(rule.getCost() >= 1);

        if (Smt::check(buildAnd(guard), its) == Smt::Sat) {
            ProofOutput::Proof.newline();
            ProofOutput::Proof.result("The following rule witnesses the lower bound Omega(1):");
            stringstream s;
            ITSExport::printLabeledRule(idx, its, s);
            ProofOutput::Proof.append(s);

            res.mutex.lock();
            res.guard = rule.getGuard();
            res.cost = rule.getCost();
            res.solvedCost = rule.getCost();
            res.cpx = Complexity::Const;
            res.mutex.unlock();
        }
    }
}


void Analysis::getMaxRuntimeOf(const set<TransIdx> &rules, RuntimeResult &res) {
    auto isTempVar = [&](const ExprSymbol &var){ return its.isTempVar(var); };

    // Only search for runtimes that improve upon the current runtime
    vector<TransIdx> todo(rules.begin(), rules.end());

    // sort the rules before analyzing them
    // non-terminating rules first
    // non-polynomial (i.e., most likely exponential) rules second (preferring rules with temporary variables)
    // rules with temporary variables (sorted by their degree) third
    // rules without temporary variables (sorted by their degree) last
    // if rules are equal wrt. the criteria above, prefer those with less constraints in the guard
    auto comp = [this, isTempVar](const TransIdx &fst, const TransIdx &snd) {
        Rule fstRule = its.getRule(fst);
        Rule sndRule = its.getRule(snd);
        Expression fstCpxExp = fstRule.getCost().expand();
        Expression sndCpxExp = sndRule.getCost().expand();
        if (fstCpxExp != sndCpxExp) {
            if (fstCpxExp.isNontermSymbol()) return true;
            if (sndCpxExp.isNontermSymbol()) return false;
            bool fstIsNonPoly = !fstCpxExp.isPolynomial();
            bool sndIsNonPoly = !sndCpxExp.isPolynomial();
            if (fstIsNonPoly > sndIsNonPoly) return true;
            if (fstIsNonPoly < sndIsNonPoly) return false;
            bool fstHasTmpVar = fstCpxExp.hasVariableWith(isTempVar);
            bool sndHasTmpVar = sndCpxExp.hasVariableWith(isTempVar);
            if (fstHasTmpVar > sndHasTmpVar) return true;
            if (fstHasTmpVar < sndHasTmpVar) return false;
            Complexity fstCpx = fstCpxExp.getComplexity();
            Complexity sndCpx = sndCpxExp.getComplexity();
            if (fstCpx > sndCpx) return true;
            if (fstCpx < sndCpx) return false;
        }
        unsigned long fstGuardSize = fstRule.getGuard().size();
        unsigned long sndGuardSize = sndRule.getGuard().size();
        return fstGuardSize < sndGuardSize;
    };

    sort(todo.begin(), todo.end(), comp);

    for (TransIdx ruleIdx : todo) {
        Rule &rule = its.getRuleMut(ruleIdx);

        // getComplexity() is not sound, but gives an upperbound, so we can avoid useless asymptotic checks.
        // We have to be careful with temp variables, since they can lead to unbounded cost.
        const Expression &cost = rule.getCost();
        bool hasTempVar = !cost.isNontermSymbol() && cost.hasVariableWith(isTempVar);

        if (cost.getComplexity() <= max(res.cpx, Complexity::Const) && !hasTempVar) {
            continue;
        }

        ProofOutput::Proof.section(stringstream() << "Computing asymptotic complexity for rule " << ruleIdx);

        // Simplify guard to speed up asymptotic check
        bool simplified = false;
        simplified |= Preprocess::simplifyGuard(rule.getGuardMut());
        simplified |= Preprocess::simplifyGuardBySmt(rule.getGuardMut(), its);
        if (simplified) {
            ProofOutput::Proof.append("Simplified the guard:");
            stringstream s;
            ITSExport::printLabeledRule(ruleIdx, its, s);
            ProofOutput::Proof.append(s);
        }

        // Perform the asymptotic check to verify that this rule's guard allows infinitely many models
        option<AsymptoticBound::Result> checkRes;
        bool isPolynomial = rule.getCost().isPolynomial() && !rule.getCost().isNontermSymbol();
        if (isPolynomial) {
            for (const Expression &e: rule.getGuard()) {
                if (!Expression(e.lhs()).isPolynomial() || !Expression(e.rhs()).isPolynomial()) {
                    isPolynomial = false;
                    break;
                }
            }
        }
        uint timeout = Timeout::soft() ? Config::Z3::LimitTimeoutFinalFast : Config::Z3::LimitTimeoutFinal;
        if (isPolynomial) {
            checkRes = AsymptoticBound::determineComplexityViaSMT(
                    its,
                    rule.getGuard(),
                    rule.getCost(),
                    true,
                    res.cpx,
                    timeout);
        } else {
            checkRes = AsymptoticBound::determineComplexity(
                    its,
                    rule.getGuard(),
                    rule.getCost(),
                    true,
                    res.cpx,
                    timeout);
        }

        if (checkRes.get().cpx > res.cpx) {
            ProofOutput::Proof.newline();
            ProofOutput::Proof.result(stringstream() << "Proved lower bound " << checkRes.get().cpx << ".");
            ProofOutput::Proof.storeSubProof(checkRes.get().proof, "limit calculus");

            res.mutex.lock();
            res.cpx = checkRes.get().cpx;
            res.solvedCost = checkRes.get().solvedCost;
            res.reducedCpx = checkRes.get().reducedCpx;
            res.guard = rule.getGuard();
            res.cost = rule.getCost();
            res.mutex.unlock();

            if (res.cpx >= Complexity::Unbounded) {
                break;
            }
        }

    }

}


void Analysis::getMaxRuntime(RuntimeResult &res) {
    auto rules = its.getTransitionsFrom(its.getInitialLocation());
    getMaxRuntimeOf(rules, res);
}


// ###############################
// ## Complexity After Timeout  ##
// ###############################

/**
 * Helper for removeConstantRulesAfterTimeout.
 * Returns true if there are no non-constant rules reachable from curr.
 */
static bool removeConstantPathsImpl(ITSProblem &its, LocationIdx curr, set<LocationIdx> &visited) {
    if (visited.insert(curr).second == false) return true; //already seen, remove any transitions forming a loop

    for (LocationIdx next : its.getSuccessorLocations(curr)) {
        // Check if all rules reachable from next have constant cost.
        // In this case, all constant rules leading to next are not interesting and can be removed.
        if (removeConstantPathsImpl(its, next, visited)) {
            for (TransIdx rule : its.getTransitionsFromTo(curr, next)) {
                if (its.getRule(rule).getCost().getComplexity() <= Complexity::Const) {
                    its.removeRule(rule);
                }
            }
        }
    }

    // If all rules have been deleted, no non-constant rules are reachable and curr is not of any interest.
    return its.getTransitionsFrom(curr).empty();
}


void Analysis::removeConstantPathsAfterTimeout() {
    set<LocationIdx> visited;
    removeConstantPathsImpl(its, its.getInitialLocation(), visited);
}


void Analysis::getMaxPartialResult(RuntimeResult &res) {
    LocationIdx initial = its.getInitialLocation(); // just a shorthand

    // contract and always compute the maximum complexity to allow abortion at any time
    while (true) {

        // check runtime of all rules from the start state
        getMaxRuntimeOf(its.getTransitionsFrom(initial), res);

        // handle special cases to ensure termination in time
        if (res.cpx >= Complexity::Unbounded) return;

        // contract next level (if there is one), so we get new rules from the start state
        auto succs = its.getSuccessorLocations(initial);
        if (succs.empty()) return;

        for (LocationIdx succ : succs) {
            for (TransIdx first : its.getTransitionsFromTo(initial,succ)) {
                for (TransIdx second : its.getTransitionsFrom(succ)) {

                    auto chained = Chaining::chainRules(its, its.getRule(first), its.getRule(second));
                    if (chained) {
                        its.addRule(chained.get());
                    }

                }

                // We already computed the complexity and tried to chain, so we can drop this rule
                its.removeRule(first);
            }
        }
        ProofOutput::Proof.headline("Performed chaining from the start location:");
    }

}
