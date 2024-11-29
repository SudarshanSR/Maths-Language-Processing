#ifndef TOKEN_H
#define TOKEN_H

#include <cmath>
#include <map>
#include <string>

#include <gsl/gsl>

namespace mlp {
class Constant;
class Variable;
class Function;
struct Term;
class Terms;
class Expression;

using Token =
    std::variant<Constant, Variable, Function, Term, Terms, Expression>;
using OwnedToken = std::unique_ptr<Token>;

[[nodiscard]] bool
is_dependent_on(Token const &token, Variable const &variable);

[[nodiscard]] bool is_linear_of(Token const &token, Variable const &variable);

[[nodiscard]] Token
evaluate(Token const &token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Token const &token);

[[nodiscard]] Token derivative(
    Token const &token, Variable const &variable, std::uint32_t order,
    std::map<Variable, Token> const &values
);

[[nodiscard]] Token
derivative(Token const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] Token integral(Token const &token, Variable const &variable);

[[nodiscard]] Token integral(
    Token const &token, Variable const &variable, Token const &from,
    Token const &to
);

enum class Sign { pos, neg };

Token tokenise(std::string expression);

[[nodiscard]] Term operator-(Token const &token);

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
} // namespace mlp

[[nodiscard]] std::string to_string(mlp::Token const &token);

std::string to_string(mlp::Sign sign);

namespace mlp {
std::ostream &operator<<(std::ostream &os, Token const &token);
} // namespace mlp

#endif