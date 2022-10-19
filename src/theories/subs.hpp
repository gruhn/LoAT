#pragma once

#include "itheory.hpp"
#include "thset.hpp"

namespace theory {

template <ITheory... Th>
class Subs {

    using TheTheory = Theory<Th...>;
    using VS = VarSet<Th...>;
    using Var = typename TheTheory::Var;
    using Lit = typename TheTheory::Lit;
    using It = typename TheTheory::Iterator;
    using Expr = typename TheTheory::Expression;

    typename TheTheory::Subs t;
    static const size_t variant_size = std::variant_size_v<Expr>;

    template <ITheory... Th_>
    friend bool operator==(const Subs<Th_...> &fst, const Subs<Th_...> &snd);

    template <ITheory... Th_>
    friend bool operator<(const Subs<Th_...> &fst, const Subs<Th_...> &snd);

public:

    using Pair = typename TheTheory::Pair;

    class Iterator {

        It begin(size_t i) const {
            return std::visit([](const auto &subs){return It(subs.begin());}, get_rt(i, subs.t));
        }

        It end(size_t i) const {
            return std::visit([](const auto &subs){return It(subs.end());}, get_rt(i, subs.t));
        }

        Pair get_current() const {
            Pair res = std::visit([](const auto &it){return Pair(*it);}, ptr);
            return res;
        }

    public:

        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Pair;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        Iterator(const Subs &subs, const It &ptr) : subs(subs), ptr(ptr), current(get_current()) {}

        reference operator*() const {
            return current;
        }

        pointer operator->() {
            return &current;
        }

        // Prefix increment
        Iterator& operator++() {
            if (ptr == end(ptr.index())) {
                if (ptr.index() + 1 == variant_size) {
                    throw std::invalid_argument("out of bounds");
                }
                ptr = begin(ptr.index() + 1);
            } else {
                ptr = std::visit([](auto &it){
                    ++it;
                    return It(it);
                }, ptr);
            }
            current = get_current();
            return *this;
        }

        // Postfix increment
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator== (const Iterator& a, const Iterator& b) {
            return a.ptr == b.ptr;
        };

        friend bool operator!= (const Iterator& a, const Iterator& b) {
            return a.ptr != b.ptr;
        };

    private:

        const Subs &subs;
        It ptr;
        Pair current;

    };

    const Iterator begin() const {
        return Iterator(*this, std::get<0>(t).begin());
    }

    const Iterator end() const {
        return Iterator(*this, std::get<variant_size - 1>(t).end());
    }

private:

    template<std::size_t I = 0>
    inline void projectImpl(Subs& res, const VS &vars) const {
        if constexpr (I < sizeof...(Th)) {
            std::get<I>(res) = std::get<I>(t).project(std::get<I>(vars.t));
            projectImpl<I+1>(res, vars);
        }
    }

public:

    Subs project(const VS &vars) const {
        Subs res;
        projectImpl(res, vars);
        return Subs(res);
    }

private:

    template<std::size_t I = 0>
    inline void putImpl(const Pair &p) {
        if constexpr (I < sizeof...(Th)) {
            if (std::holds_alternative<std::variant_alternative_t<I, Pair>>(p)) {
                const auto &[x,y] = std::get<I>(p);
                std::get<I>(t).put(x, y);
            } else {
                putImpl<I+1>(p);
            }
        }
    }

public:

    void put(const Pair &p) {
        putImpl(p);
    }

    template <ITheory T>
    void put(const typename T::Var &var, const typename T::Expression &expr) {
        std::get<typename T::Subs>(t).put(var, expr);
    }

    Subs(){}

    Subs(Pair &p) {
        put(p);
    }

    template<ITheory T>
    static Subs build(const typename T::Var var, const typename T::Expression expr) {
        Subs subs;
        subs.put<T>(var, expr);
        return subs;
    }

    Subs(typename Th::Subs... subs): t(subs...) {}

private:

    template<std::size_t I = 0>
    inline void domainImpl(VS &res) const {
        if constexpr (I < sizeof...(Th)) {
            std::get<I>(res.t) = std::get<I>(t).domain();
            _domainImpl<I+1>(res);
        }
    }

public:

    VS domain() const {
        VS res;
        domainImpl(res);
        return res;
    }

private:

    template<std::size_t I = 0>
    inline Expr subsImpl(const Lit &s) const {
        if constexpr (I < sizeof...(Th)) {
            if (std::holds_alternative<std::variant_alternative_t<I, Lit>>(s)) {
                return std::get<I>(s).subs(std::get<I>(t));
            } else {
                return subsImpl<I+1>(s);
            }
        } else {
            return s;
        }
    }

public:

    Expr subs(const Lit &s) const {
        return subsImpl(s);
    }

private:

    template<std::size_t I = 0>
    inline void collectVarsImpl(VS &res) const {
        if constexpr (I < sizeof...(Th)) {
            std::get<I>(t).collectVars(res.template get<I>());
            collectVarsImpl<I+1>(res);
        }
    }

public:

    void collectVars(VS &vars) const {
        collectVarsImpl(vars);
    }

private:

    template<std::size_t I = 0>
    inline size_t hashImpl() const {
        if constexpr (I + 1 >= sizeof...(Th)) {
            return std::get<I>(t).hash();
        } else {
            size_t res = hashImpl<I+1>();
            boost::hash_combine<size_t>(res, std::get<I>(t).hash());
            return res;
        }
    }

public:

    size_t hash() const {
        return hashImpl();
    }

private:

    template<std::size_t I = 0>
    inline Expr getImpl(const Var &var) const {
        if constexpr (I >= sizeof...(Th)) {
            throw std::invalid_argument("variable not found");
        } else if (std::holds_alternative<std::variant_alternative_t<I, Var>>(var)) {
            return std::get<I>(t).get(std::get<I>(var));
        } else {
            return getImpl<I+1>(var);
        }
    }

public:

    Expr get(const Var &var) const {
        return getImpl(var);
    }

    template <ITheory T>
    typename T::Expression get(const typename T::Var &var) const {
        return std::get<typename T::Subs>(t).get(var);
    }

private:

    template<std::size_t I = 0>
    inline void composeImpl(const Subs &that) {
        if constexpr (I < sizeof...(Th)) {
            std::get<I>(t).compose(std::get<I>(that.t));
            composeImpl<I+1>(that);
        }
    }

public:

    Subs compose(const Subs &that) const {
        Subs res(*this);
        res.composeImpl(that);
        return res;
    }

private:

    template<std::size_t I = 0>
    inline void concatImpl(const Subs &that) {
        if constexpr (I < sizeof...(Th)) {
            std::get<I>(t).compose(std::get<I>(that.t));
            concatImpl<I+1>(that);
        }
    }

public:

    Subs concat(const Subs &that) const {
        Subs res(*this);
        res.concatImpl(that);
        return res;
    }

private:

    template<std::size_t I = 0>
    inline bool changesImpl(const Var &x) const {
        if constexpr (I < sizeof...(Th)) {
            if (std::holds_alternative<std::variant_alternative_t<I, Var>>(x)) {
                return std::get<I>(t).changes(std::get<I>(x));
            } else {
                return changesImpl<I+1>(x);
            }
        } else {
            return false;
        }
    }

public:

    bool changes(const Var &x) const {
        return changesImpl(x);
    }

private:

    template<std::size_t I = 0>
    inline void eraseImpl(const Var &x) {
        if constexpr (I < sizeof...(Th)) {
            if (std::holds_alternative<std::variant_alternative_t<I, Var>>(x)) {
                std::get<I>(t).erase(std::get<I>(x));
            } else {
                eraseImpl<I+1>(x);
            }
        }
    }

public:

    void erase(const Var &x) {
        eraseImpl(x);
    }

private:

    template<std::size_t I = 0>
    inline void printImpl(std::ostream &s) const {
        if constexpr (I < sizeof...(Th)) {
            s << std::get<I>(t);
            if constexpr (I + 1 < variant_size) {
                printImpl<I+1>(s);
            }
        }
    }

public:

    void print(std::ostream &s) const {
        printImpl(s);
    }

private:

    template<std::size_t I = 0>
    inline Iterator findImpl(const Var &var) const {
        if constexpr (I < sizeof...(Th)) {
            if (std::holds_alternative<std::variant_alternative_t<I, Var>>(var)) {
                const auto &subs = std::get<I>(t);
                const auto &it = subs.find(std::get<I>(var));
                if (it == subs.end()) {
                    return end();
                } else {
                    return Iterator(*this, it);
                }
            } else {
                return findImpl<I+1>(var);
            }
        } else {
            return end();
        }
    }

public:

    Iterator find(const Var &var) const {
        return findImpl(var);
    }

    bool contains(const Var &var) const {
        return find(var) != end();
    }

    size_t size() const {
        return std::apply([](const auto&... x){return (0 + ... + x.size());}, t);
    }

    bool empty() const {
        return std::apply([](const auto&... x){return (true && ... && x.empty());}, t);
    }

    bool isLinear() const {
        return std::apply([](const auto&... x){return (true && ... && x.isLinear());}, t);
    }

    bool isPoly() const {
        return std::apply([](const auto&... x){return (true && ... && x.isPoly());}, t);
    }

    int compare(const Subs &that) const {
        if (this->t == that.t) {
            return 0;
        } else if (this->t < that.t) {
            return -1;
        } else {
            return 1;
        }
    }

    template <ITheory T>
    typename T::Subs& get() {
        return std::get<typename T::Subs>(t);
    }

    template <ITheory T>
    const typename T::Subs& get() const {
        return std::get<typename T::Subs>(t);
    }

    template <size_t I>
    std::tuple_element_t<I, decltype(t)>& get() {
        return std::get<I>(t);
    }

    template <size_t I>
    const std::tuple_element_t<I, decltype(t)>& get() const {
        return std::get<I>(t);
    }

};

}
