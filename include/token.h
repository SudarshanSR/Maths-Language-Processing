#ifndef TOKEN_H
#define TOKEN_H

#include <cmath>
#include <map>
#include <ostream>
#include <string>

#include <gsl/gsl>

namespace mlp {
struct Token;

template <typename T> using Owned = std::unique_ptr<T>;

using SharedToken = std::shared_ptr<Token>;
using OwnedToken = Owned<Token>;

class Variable;

struct Term;

struct Token {
    virtual ~Token() = default;

    [[nodiscard]] virtual gsl::owner<Token *> clone() const = 0;

    [[nodiscard]] virtual gsl::owner<Token *> move() && = 0;

    explicit virtual operator std::string() const = 0;

    [[nodiscard]] Term operator-() const;
};

class Constant;
class Function;
class Terms;
class Expression;

using token =
    std::variant<Constant, Variable, Function, Term, Terms, Expression>;

[[nodiscard]] bool
is_dependent_on(token const &token, Variable const &variable);

[[nodiscard]] bool is_linear_of(token const &token, Variable const &variable);

[[nodiscard]] token
evaluate(token const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] token simplified(Token const &token);

[[nodiscard]] token simplified(token const &token);

[[nodiscard]] token derivative(
    token const &token, Variable const &variable, std::uint32_t order,
    std::map<Variable, SharedToken> const &values
);

[[nodiscard]] token
derivative(token const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] token integral(Token const &token, Variable const &variable);

[[nodiscard]] token integral(token const &token, Variable const &variable);

[[nodiscard]] token integral(
    Token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
);

[[nodiscard]] token integral(
    token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
);

enum class Sign { pos, neg };

OwnedToken tokenise(std::string expression);

template <typename T>
    requires std::is_base_of_v<Token, T>
std::ostream &operator<<(std::ostream &os, T const &token) {
    os << static_cast<std::string>(token);

    return os;
}

[[nodiscard]] Term operator*(std::double_t lhs, Token const &rhs);

[[nodiscard]] Term operator*(Token const &lhs, std::double_t rhs);

[[nodiscard]] Term operator*(std::double_t lhs, Token &&rhs);

[[nodiscard]] Term operator*(Token &&lhs, std::double_t rhs);

[[nodiscard]] Expression operator+(Token const &lhs, Token const &rhs);

[[nodiscard]] Expression operator-(Token const &lhs, Token const &rhs);

[[nodiscard]] Expression operator*(std::double_t lhs, Expression rhs);

[[nodiscard]] Terms operator*(Token const &lhs, Token const &rhs);

[[nodiscard]] Expression operator*(Token const &lhs, Expression rhs);

[[nodiscard]] Terms operator/(Token const &lhs, Token const &rhs);

Term operator^(Token const &lhs, std::double_t rhs);

Term operator^(Token const &lhs, Token const &rhs);

Term operator^(Token const &lhs, Token &&rhs);

Term operator^(Token &&lhs, std::double_t rhs);

Term operator^(Token &&lhs, Token const &rhs);

Term operator^(Token &&lhs, Token &&rhs);

std::ostream &operator<<(std::ostream &os, Sign sign);

[[nodiscard]] token to_variant(Token const &token);

[[nodiscard]] Token const &from_variant(token const &token);

[[nodiscard]] Token &&from_variant(token &&token);
} // namespace mlp

std::string to_string(mlp::Sign sign);

#endif