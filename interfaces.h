#ifndef INTERFACES_H
#define INTERFACES_H

#include <map>

namespace mlp {
template <typename T> struct Dependable {
    virtual ~Dependable() = default;

    [[nodiscard]] virtual bool is_dependent_on(T const &param) const = 0;

    [[nodiscard]] virtual bool is_linear_of(T const &param) const = 0;
};

template <typename Key, typename Value, typename Result> struct Evaluatable {
    virtual ~Evaluatable() = default;

    [[nodiscard]] virtual Result
    evaluate(std::map<Key, Value> const &values) const = 0;
};

template <typename Result> struct Simplifiable {
    virtual ~Simplifiable() = default;

    [[nodiscard]] virtual Result simplified() const = 0;
};
} // namespace mlp

#endif // INTERFACES_H
