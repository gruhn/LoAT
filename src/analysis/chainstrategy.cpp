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

#include "chainstrategy.hpp"
#include "chain.hpp"

ResultViaSideEffects Chaining::chainLinearPaths(ITSProblem &its) {
    ResultViaSideEffects res;
    for (const auto first_idx: its.getAllTransitions()) {
        const auto succ {its.getSuccessors(first_idx)};
        if (succ.size() == 1 && succ.find(first_idx) == succ.end()) {
            const auto second_idx {*succ.begin()};
            const auto &first {its.getRule(first_idx)};
            auto second {its.getRule(second_idx)};
            const auto chained {chain(first, second, its)};
            res.succeed();
            res.chainingProof(first, second, chained);
            its.addRule(chained, first_idx, second_idx);
            its.removeRule(first_idx);
            if (its.getPredecessors(second_idx).empty()) {
                its.removeRule(second_idx);
            }
            res.deletionProof({first_idx});
        }
    }
    return res;
}
