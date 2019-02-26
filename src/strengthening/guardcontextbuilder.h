//
// Created by ffrohn on 2/25/19.
//

#ifndef LOAT_STRENGTHENING_GUARD_CONTEXT_BUILDER_H
#define LOAT_STRENGTHENING_GUARD_CONTEXT_BUILDER_H


#include <its/types.h>
#include "types.h"

namespace strengthening {

    class GuardContextBuilder {

    public:

        static const GuardContext build(const GuardList &guard, const std::vector<GiNaC::exmap> &updates);

    private:

        const GuardList &guard;
        const std::vector<GiNaC::exmap> &updates;

        GuardContextBuilder(const GuardList &guard, const std::vector<GiNaC::exmap> &updates);

        const GuardContext build() const;

        const GuardList computeConstraints() const;

        const Result splitInvariants(const GuardList &constraints) const;

        const Result splitMonotonicConstraints(
                const GuardList &invariants,
                const GuardList &nonInvariants) const;

    };

}

#endif //LOAT_STRENGTHENING_GUARD_CONTEXT_BUILDER_H
