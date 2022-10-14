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

#include "variablemanager.hpp"
#include "exceptions.hpp"

using namespace std;


std::recursive_mutex VariableManager::mutex;

bool VariableManager::isTempVar(const Var &var) const {
    std::lock_guard guard(mutex);
    return temporaryVariables.find(theory::getName(var)) != temporaryVariables.end();
}

void VariableManager::toLower(string &str) const {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

string VariableManager::getFreshName(string basename) {
    std::lock_guard guard(mutex);
    toLower(basename);
    std::string res;
    if (basenameCount.find(basename) == basenameCount.end()) {
        basenameCount.emplace(basename, 0);
        res = basename;
    } else {
        unsigned int count = basenameCount.at(basename);
        res = basename + to_string(count);
        while (used.find(res) != used.end()) {
            ++count;
            res = basename + to_string(count);
        }
        basenameCount[basename] = count + 1;
    }
    used.insert(res);
    return res;
}

VarSet VariableManager::getVars() const {
    std::lock_guard guard(mutex);
    return variables;
}

option<Var> VariableManager::getVar(std::string name) const {
    std::lock_guard guard(mutex);
    toLower(name);
    auto it = variableNameLookup.find(name);
    if (it == variableNameLookup.end()) {
        return {};
    } else {
        return it->second;
    }
}

Expr::Type VariableManager::getType(const Var &x) const {
    std::lock_guard guard(mutex);
    if (untrackedVariables.find(x) != untrackedVariables.end()) {
        return untrackedVariables.at(x);
    } else {
        return Expr::Int;
    }
}

//std::pair<QFormula, Subs> VarMan::normalizeVariables(const QFormula &f) {
//    VarSet vars;
//    const auto matrix = f.getMatrix();
//    matrix->collectVars(vars);
//    Subs normalization, inverse;
//    unsigned count = 0;
//    for (const Var &x: vars) {
//        std::string varName = "x" + std::to_string(count);
//        option<Var> replacement = getVar(varName);
//        if (!replacement) replacement = addFreshTemporaryVariable(varName);
//        ++count;
//        normalization.put(x, *replacement);
//        inverse.put(*replacement, x);
//    }
//    const auto newMatrix = matrix->subs(normalization);
//    std::vector<Quantifier> newPrefix;
//    for (const auto& q: f.getPrefix()) {
//        std::set<Var> newVars;
//        std::map<NumVar, Expr> newLowerBounds;
//        std::map<NumVar, Expr> newUpperBounds;
//        for (const auto& x: q.getVars()) {
//            if (vars.find(x) != vars.end()) {
//                newVars.insert(normalization.get(x).toVar());
//                auto lb = q.lowerBound(x);
//                auto ub = q.upperBound(x);
//                if (lb) {
//                    newLowerBounds[x] = lb.get();
//                }
//                if (ub) {
//                    newUpperBounds[x] = ub.get();
//                }
//            }
//        }
//        if (!newVars.empty()) {
//            newPrefix.push_back(Quantifier(q.getType(), newVars, newLowerBounds, newUpperBounds));
//        }
//    }
//    return {QuantifiedFormula(newPrefix, newMatrix), inverse};
//}
