#ifndef DERIVATIVE_H
#define DERIVATIVE_H

#include <map>
#include <memory>

struct Token;
struct Variable;

using OwnedToken = std::unique_ptr<Token>;
using SharedToken = std::shared_ptr<Token>;

struct Differentiable {
    virtual ~Differentiable() = default;

    [[nodiscard]] virtual OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const = 0;

    [[nodiscard]] OwnedToken derivative(
        Variable const &variable, std::uint32_t order,
        std::map<Variable, SharedToken> const &values
    ) const;
};

#endif // DERIVATIVE_H
