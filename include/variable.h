#ifndef VARIABLE_H
#define VARIABLE_H

#include "token.h"

namespace mlp {
class Variable final {
    char var{'\0'};

  public:
    Constant coefficient{1};

    Variable() = default;

    Variable(Constant coefficient, char var);

    explicit Variable(char var);

    Variable(Variable const &) = default;

    Variable(Variable &&) = default;

    Variable &operator=(Variable const &) = default;

    Variable &operator=(Variable &&) = default;

    explicit operator std::string() const;

    [[nodiscard]] Variable operator-() const;

    [[nodiscard]] bool operator<(Variable) const;

    [[nodiscard]] bool operator==(Variable) const;

    Variable &operator*=(Constant rhs);

    Variable &operator/=(Constant rhs);

    friend std::istream &operator>>(std::istream &input, Variable &output);
};

[[nodiscard]] bool is_dependent_on(Variable token, Variable variable);

[[nodiscard]] bool is_linear_of(Variable token, Variable variable);

[[nodiscard]] Token
evaluate(Variable token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Variable token);

[[nodiscard]] Token
derivative(Variable token, Variable variable, std::uint32_t order);

[[nodiscard]] Token integral(Variable token, Variable variable);

[[nodiscard]] Token operator+(Variable lhs, Constant rhs);
[[nodiscard]] Token operator+(Variable lhs, Variable rhs);
[[nodiscard]] Token operator+(Variable lhs, Function const &rhs);
[[nodiscard]] Token operator+(Variable lhs, Term const &rhs);
[[nodiscard]] Token operator+(Variable lhs, Terms const &rhs);
[[nodiscard]] Token operator+(Variable lhs, Expression rhs);

[[nodiscard]] Token operator-(Variable lhs, Constant rhs);
[[nodiscard]] Token operator-(Variable lhs, Variable rhs);
[[nodiscard]] Token operator-(Variable lhs, Function const &rhs);
[[nodiscard]] Token operator-(Variable lhs, Term const &rhs);
[[nodiscard]] Token operator-(Variable lhs, Terms const &rhs);
[[nodiscard]] Token operator-(Variable lhs, Expression rhs);

[[nodiscard]] Token operator*(Variable lhs, Constant rhs);
[[nodiscard]] Token operator*(Variable lhs, Variable rhs);
[[nodiscard]] Token operator*(Variable lhs, Function const &rhs);
[[nodiscard]] Token operator*(Variable lhs, Term const &rhs);
[[nodiscard]] Token operator*(Variable lhs, Terms rhs);
[[nodiscard]] Token operator*(Variable lhs, Expression const &rhs);

[[nodiscard]] Token operator/(Variable lhs, Constant rhs);
[[nodiscard]] Token operator/(Variable lhs, Variable rhs);
[[nodiscard]] Token operator/(Variable lhs, Function const &rhs);
[[nodiscard]] Token operator/(Variable lhs, Term const &rhs);
[[nodiscard]] Token operator/(Variable lhs, Terms const &rhs);
[[nodiscard]] Token operator/(Variable lhs, Expression const &rhs);

[[nodiscard]] Token pow(Variable lhs, Constant rhs);
[[nodiscard]] Token pow(Variable lhs, Function rhs);
[[nodiscard]] Token pow(Variable lhs, Term rhs);
[[nodiscard]] Token pow(Variable lhs, Terms rhs);
} // namespace mlp

#endif // VARIABLE_H
