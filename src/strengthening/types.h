//
// Created by ffrohn on 2/18/19.
//

#ifndef LOAT_STRENGTHENING_TYPES_H
#define LOAT_STRENGTHENING_TYPES_H


#include <its/types.h>
#include <rule.h>

namespace strengthening {

    struct Context {

        Context(const Rule &rule,
                std::vector<GiNaC::exmap> updates,
                GuardList invariants,
                GuardList todo,
                std::vector<GuardList> preconditions,
                VariableManager &varMan):
                rule(rule),
                updates(std::move(updates)),
                invariants(std::move(invariants)),
                todo(std::move(todo)),
                preconditions(std::move(preconditions)),
                varMan(varMan) { }

        const Rule &rule;
        const std::vector<GiNaC::exmap> updates;
        const GuardList invariants;
        const GuardList todo;
        const std::vector<GuardList> preconditions;
        VariableManager &varMan;
    };

    struct Implication {
        GuardList premise;
        GuardList conclusion;
    };

    struct Result {
        GuardList solved;
        GuardList failed;
    };

    struct Invariants {
        GuardList invariants;
        GuardList pseudoInvariants;
    };

    struct MaxSmtConstraints {
        std::vector<z3::expr> hard;
        std::vector<z3::expr> soft;
    };

    struct Initiation {
        std::vector<z3::expr> valid;
        std::vector<z3::expr> satisfiable;
    };

    struct SmtConstraints {

        SmtConstraints(
                Initiation initiation,
                std::vector<z3::expr> templatesInvariant,
                std::vector<z3::expr> conclusionsInvariant,
                std::vector<z3::expr> conclusionsMonotonic) :
                initiation(std::move(initiation)),
                templatesInvariant(std::move(templatesInvariant)),
                conclusionsInvariant(std::move(conclusionsInvariant)),
                conclusionsMonotonic(std::move(conclusionsMonotonic)) {}

        const Initiation initiation;
        const std::vector<z3::expr> templatesInvariant;
        const std::vector<z3::expr> conclusionsInvariant;
        const std::vector<z3::expr> conclusionsMonotonic;

    };

    typedef std::function<const MaxSmtConstraints(const SmtConstraints &, Z3Context &)> Mode;

}

#endif //LOAT_STRENGTHENING_TYPES_H
