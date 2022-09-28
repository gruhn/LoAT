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

using namespace std;


std::recursive_mutex VariableManager::mutex;

bool VariableManager::isTempVar(const Var &var) const {
    std::lock_guard guard(mutex);
    return temporaryVariables.find(var.get_name()) != temporaryVariables.end();
}

bool VariableManager::isTempVar(const BoolVar &var) const {
    std::lock_guard guard(mutex);
    return temporaryVariables.find(var.getName()) != temporaryVariables.end();
}

Var VariableManager::addFreshVariable(string basename) {
    std::lock_guard guard(mutex);
    return addVariable(getFreshName(basename));
}

BoolVar VariableManager::addFreshBoolVariable(string basename) {
    std::lock_guard guard(mutex);
    return addBoolVariable(getFreshName(basename));
}

Var VariableManager::addFreshTemporaryVariable(string basename) {
    std::lock_guard guard(mutex);
    Var x = addVariable(getFreshName(basename));
    temporaryVariables.insert(x.get_name());
    return x;
}

BoolVar VariableManager::addFreshTemporaryBoolVariable(string basename) {
    std::lock_guard guard(mutex);
    BoolVar x = addBoolVariable(getFreshName(basename));
    temporaryVariables.insert(x.getName());
    return x;
}

Var VariableManager::getFreshUntrackedSymbol(string basename, Expr::Type type) {
    std::lock_guard guard(mutex);
    Var res(getFreshName(basename));
    variableNameLookup.emplace(res.get_name(), res);
    untrackedVariables[res] = type;
    return res;
}

BoolVar VariableManager::getFreshUntrackedBoolSymbol(string basename) {
    std::lock_guard guard(mutex);
    BoolVar res(getFreshName(basename));
    return res;
}

void toLower(string &str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

Var VariableManager::addVariable(string name) {
    std::lock_guard guard(mutex);
    toLower(name);
    auto sym = Var(name);
    variables.insert(sym);
    variableNameLookup.emplace(name, sym);
    return sym;
}

BoolVar VariableManager::addBoolVariable(string name) {
    std::lock_guard guard(mutex);
    toLower(name);
    auto sym = BoolVar(name);
    boolVariables.insert(sym);
    boolVariableNameLookup.emplace(name, sym);
    return sym;
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

BoolVarSet VariableManager::getBoolVars() const {
    std::lock_guard guard(mutex);
    return boolVariables;
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

option<BoolVar> VariableManager::getBoolVar(std::string name) const {
    std::lock_guard guard(mutex);
    toLower(name);
    auto it = boolVariableNameLookup.find(name);
    if (it == boolVariableNameLookup.end()) {
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

