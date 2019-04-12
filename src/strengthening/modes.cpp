//
// Created by ffrohn on 2/21/19.
//

#include "modes.hpp"
#include "../z3/z3context.hpp"

namespace strengthening {

    typedef Modes Self;

    const std::vector<Mode> Self::invarianceModes() {
        return {invariance, pseudoInvariance};
    }

    const std::vector<Mode> Self::modes() {
        return {invariance, monotonicity, pseudoInvariance, pseudoMonotonicity};
    }

    const MaxSmtConstraints Self::invariance(const SmtConstraints &constraints, Z3Context &z3Ctx) {
        MaxSmtConstraints res;
        res.hard.insert(res.hard.end(), constraints.initiation.valid.begin(), constraints.initiation.valid.end());
        res.hard.insert(res.hard.end(), constraints.initiation.satisfiable.begin(), constraints.initiation.satisfiable.end());
        z3::expr_vector someConclusionInvariant(z3Ctx);
        for (const z3::expr &e: constraints.conclusionsInvariant) {
            res.soft.push_back(e);
            someConclusionInvariant.push_back(e);
        }
        res.hard.push_back(z3::mk_or(someConclusionInvariant));
        res.hard.insert(res.hard.end(), constraints.templatesInvariant.begin(), constraints.templatesInvariant.end());
        return res;
    }

    const MaxSmtConstraints Self::pseudoInvariance(const SmtConstraints &constraints, Z3Context &z3Ctx) {
        MaxSmtConstraints res;
        z3::expr_vector satisfiableForSomePredecessor(z3Ctx);
        for (const z3::expr &e: constraints.initiation.satisfiable) {
            satisfiableForSomePredecessor.push_back(e);
        }
        res.hard.push_back(z3::mk_or(satisfiableForSomePredecessor));
        z3::expr_vector someConclusionInvariant(z3Ctx);
        for (const z3::expr &e: constraints.conclusionsInvariant) {
            res.soft.push_back(e);
            someConclusionInvariant.push_back(e);
        }
        // res.soft.insert(res.soft.end(), constraints.initiation.valid.begin(), constraints.initiation.valid.end());
        res.hard.push_back(z3::mk_or(someConclusionInvariant));
        res.hard.insert(res.hard.end(), constraints.templatesInvariant.begin(), constraints.templatesInvariant.end());
        return res;
    }

    const MaxSmtConstraints Self::monotonicity(const SmtConstraints &constraints, Z3Context &z3Ctx) {
        MaxSmtConstraints res;
        res.hard.insert(res.hard.end(), constraints.initiation.valid.begin(), constraints.initiation.valid.end());
        res.hard.insert(res.hard.end(), constraints.initiation.satisfiable.begin(), constraints.initiation.satisfiable.end());
        z3::expr_vector someConclusionMonotonic(z3Ctx);
        for (const z3::expr &e: constraints.conclusionsMonotonic) {
            res.soft.push_back(e);
            someConclusionMonotonic.push_back(e);
        }
        res.hard.push_back(z3::mk_or(someConclusionMonotonic));
        res.hard.insert(res.hard.end(), constraints.templatesInvariant.begin(), constraints.templatesInvariant.end());
        return res;
    }

    const MaxSmtConstraints Self::pseudoMonotonicity(const SmtConstraints &constraints, Z3Context &z3Ctx) {
        MaxSmtConstraints res;
        z3::expr_vector satisfiableForSomePredecessor(z3Ctx);
        for (const z3::expr &e: constraints.initiation.satisfiable) {
            satisfiableForSomePredecessor.push_back(e);
        }
        res.hard.push_back(z3::mk_or(satisfiableForSomePredecessor));
        z3::expr_vector someConclusionMonotonic(z3Ctx);
        for (const z3::expr &e: constraints.conclusionsMonotonic) {
            res.soft.push_back(e);
            someConclusionMonotonic.push_back(e);
        }
        // res.soft.insert(res.soft.end(), constraints.initiation.valid.begin(), constraints.initiation.valid.end());
        res.hard.push_back(z3::mk_or(someConclusionMonotonic));
        res.hard.insert(res.hard.end(), constraints.templatesInvariant.begin(), constraints.templatesInvariant.end());
        return res;
    }

}
