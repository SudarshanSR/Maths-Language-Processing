#ifndef DERIVATIVE_H
#define DERIVATIVE_H

#include <map>
#include <memory>

struct Token;
struct Variable;

struct Differentiable {
    virtual ~Differentiable() = default;

    [[nodiscard]] virtual std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const = 0;

    [[nodiscard]] std::shared_ptr<Token> derivative(
        Variable const &variable, std::uint32_t order,
        std::map<Variable, std::shared_ptr<Token>> const &values
    ) const;
};

#endif // DERIVATIVE_H
