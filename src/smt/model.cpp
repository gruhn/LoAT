#include "model.hpp"

Model::Model(VarMap<GiNaC::numeric> vars): vars(vars) {}

GiNaC::numeric Model::get(const Var &var) const {
    return vars.at(var);
}

bool Model::contains(const Var &var) const {
    return vars.count(var) > 0;
}

std::ostream& operator<<(std::ostream &s, const Model &e) {
    bool first = true;
    s << "{";
    for (const auto &p: e.vars) {
        if (first) {
            s << p.first << "=" << p.second;
            first = false;
        } else s << ", " << p.first << "=" << p.second;
    }
    return s << " }";
}
