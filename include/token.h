#ifndef TOKEN_H
#define TOKEN_H

#include "interfaces.h"

#include <cmath>
#include <map>
#include <optional>
#include <ostream>
#include <string>

#include <gsl/gsl>

namespace mlp {
struct Token;

template <typename T> using Owned = std::unique_ptr<T>;

using SharedToken = std::shared_ptr<Token>;
using OwnedToken = Owned<Token>;

class Variable;

struct Differentiable {
    virtual ~Differentiable() = default;

    [[nodiscard]] OwnedToken derivative(
        Variable const &variable, std::uint32_t order,
        std::map<Variable, SharedToken> const &values
    ) const;

    [[nodiscard]] virtual OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const = 0;
};

struct Integrable {
    virtual ~Integrable() = default;

    [[nodiscard]] OwnedToken integral(
        Variable const &variable, SharedToken const &from, SharedToken const &to
    ) const;

    [[nodiscard]] virtual OwnedToken
    integral(Variable const &variable) const = 0;
};

struct Term;

struct Token : Dependable<Variable>,
               Evaluatable<Variable, SharedToken, OwnedToken>,
               Simplifiable<OwnedToken>,
               Differentiable,
               Integrable {
    [[nodiscard]] virtual gsl::owner<Token *> clone() const = 0;

    [[nodiscard]] virtual gsl::owner<Token *> move() && = 0;

    explicit virtual operator std::string() const = 0;

    friend Term operator*(std::double_t lhs, Token const &rhs);

    friend Term operator*(Token const &lhs, std::double_t rhs);

    friend Term operator*(std::double_t lhs, Token &&rhs);

    friend Term operator*(Token &&lhs, std::double_t rhs);
};

enum class Sign { pos, neg };

OwnedToken tokenise(std::string expression);

template <typename T>
    requires std::is_base_of_v<Token, T>
std::ostream &operator<<(std::ostream &os, T const &token) {
    os << static_cast<std::string>(token);

    return os;
}

class Expression;

[[nodiscard]] Expression operator+(Token const &lhs, Token const &rhs);

[[nodiscard]] Expression operator-(Token const &lhs, Token const &rhs);

class Terms;

[[nodiscard]] Terms operator*(Token const &lhs, Token const &rhs);

[[nodiscard]] Expression operator*(Token const &lhs, Expression rhs);

[[nodiscard]] Terms operator/(Token const &lhs, Token const &rhs);

Term operator^(Token const &lhs, std::double_t rhs);

Term operator^(Token const &lhs, Token const &rhs);

Term operator^(Token const &lhs, OwnedToken &&rhs);

Term operator^(Token &&lhs, std::double_t rhs);

Term operator^(Token &&lhs, Token const &rhs);

Term operator^(Token &&lhs, OwnedToken &&rhs);

std::ostream &operator<<(std::ostream &os, Sign sign);
} // namespace mlp

std::string to_string(mlp::Sign sign);

#endif