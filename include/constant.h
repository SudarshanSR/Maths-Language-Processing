#ifndef CONSTANT_H
#define CONSTANT_H

#include <cmath>
#include <map>
#include <variant>

namespace mlp {
using Constant = std::double_t;
class Variable;
class Function;
struct Term;
class Terms;
class Expression;

using Token =
    std::variant<Constant, Variable, Function, Term, Terms, Expression>;

[[nodiscard]] bool is_dependent_on(Constant token, Variable variable);

[[nodiscard]] bool is_linear_of(Constant token, Variable variable);

[[nodiscard]] Token
evaluate(Constant token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Constant token);

[[nodiscard]] Token
derivative(Constant token, Variable variable, std::uint32_t order);

[[nodiscard]] Token integral(Constant token, Variable variable);

[[nodiscard]] Token operator+(Constant lhs, Token const &rhs);

[[nodiscard]] Token operator-(Constant lhs, Token const &rhs);

[[nodiscard]] Token operator*(Token const &lhs, Constant rhs);
[[nodiscard]] Token operator*(Constant lhs, Token const &rhs);
[[nodiscard]] Token operator*(Constant lhs, Expression rhs);

[[nodiscard]] Token operator/(Token const &lhs, Constant rhs);
[[nodiscard]] Token operator/(Constant lhs, Token const &rhs);
[[nodiscard]] Token operator/(Constant lhs, Variable rhs);
[[nodiscard]] Token operator/(Constant lhs, Function const &rhs);
[[nodiscard]] Token operator/(Constant lhs, Term const &rhs);
[[nodiscard]] Token operator/(Constant lhs, Terms const &rhs);
[[nodiscard]] Token operator/(Constant lhs, Expression const &rhs);

using std::pow;
[[nodiscard]] Token pow(Constant lhs, Variable rhs);
[[nodiscard]] Token pow(Constant lhs, Function rhs);
[[nodiscard]] Token pow(Constant lhs, Term rhs);
[[nodiscard]] Token pow(Constant lhs, Terms rhs);
} // namespace mlp

#endif // CONSTANT_H
