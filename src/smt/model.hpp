#pragma once

#include "itheory.hpp"
#include "thset.hpp"

template <ITheory... Th>
class Model
{

    using TheTheory = Theory<Th...>;
    using Theories = typename TheTheory::Theories;
    using S = theory::Subs<Th...>;
    using VarSet = typename theory::VarSet<Th...>;

public:

    Model() {}
    Model(const typename TheTheory::Model &m): m(m) {}

    template <ITheory T>
    typename T::Val get(const typename T::Var &var) const {
        return std::get<std::map<typename T::Var, typename T::Val>>(m).at(var);
    }

    template <ITheory T>
    void put(const typename T::Var &var, const typename T::Val &val) {
        std::get<std::map<typename T::Var, typename T::Val>>(m).emplace(var, val);
    }

    template <ITheory T>
    bool contains(const typename T::Var &var) const {
        const auto &map = std::get<std::map<typename T::Var, typename T::Val>>(m);
        return map.find(var) != map.end();
    }

private:

    template<std::size_t I = 0>
    inline void toSubsImpl(S &subs) const {
        if constexpr (I < std::tuple_size_v<Theories>) {
            const auto map = std::get<I>(m);
            for (const auto &p: map) {
                using T = std::tuple_element_t<I, Theories>;
                subs.put(typename S::Pair(std::pair(p.first, T::valToExpr(p.second))));
            }
            toSubsImpl<I+1>(subs);
        }
    }

public:

    S toSubs() const {
        S res;
        toSubsImpl(res);
        return res;
    }

private:

    template<std::size_t I = 0>
    inline void toSubsImpl(S &subs, const VarSet &vars) const {
        if constexpr (I < std::tuple_size_v<Theories>) {
            const auto &map = std::get<I>(m);
            auto &s = subs.template get<I>();
            for (const auto &x: vars.template get<I>()) {
                const auto it = map.find(x);
                if (it != map.end()) {
                    using T = std::tuple_element_t<I, Theories>;
                    s.put(x, T::valToExpr(it->second));
                }
            }
            toSubsImpl<I+1>(subs, vars);
        }
    }

public:

    S toSubs(const VarSet &vars) const {
        S res;
        toSubsImpl(res, vars);
        return res;
    }

    template <ITheory... Th_>
    friend std::ostream& operator<<(std::ostream &s, const Model<Th_...> &e);

private:

    typename TheTheory::Model m;

};

template <ITheory... Th>
std::ostream& operator<<(std::ostream &s, const Model<Th...> &e) {
    return s << e.toSubs();
}
