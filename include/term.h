#ifndef TERM_H
#define TERM_H

#include "token.h"

namespace mlp {
struct Term final {
    Constant coefficient{1};
    OwnedToken base;
    OwnedToken power;

    Term(Constant coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(Constant coefficient, Token const &base, Token const &power);

    Term(OwnedToken &&base, OwnedToken &&power);

    Term(Token const &base, Token const &power);

    Term(Term const &term);

    Term(Term &&) = default;

    Term &operator=(Term const &term);

    Term &operator=(Term &&) = default;

    explicit operator std::string() const;

    [[nodiscard]] Term operator-() const;

    Term &operator*=(Constant rhs);

    Term &operator/=(Constant rhs);

    [[nodiscard]] bool operator==(Term const &) const;
};

[[nodiscard]] bool is_dependent_on(Term const &token, Variable const &variable);

[[nodiscard]] bool is_linear_of(Term const &token, Variable const &variable);

[[nodiscard]] Token
evaluate(Term const &token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Term const &token);

[[nodiscard]] Token
derivative(Term const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] Token integral(Term const &token, Variable const &variable);

[[nodiscard]] Token operator+(Term lhs, Constant rhs);
[[nodiscard]] Token operator+(Term lhs, Variable rhs);
[[nodiscard]] Token operator+(Term const &lhs, Function rhs);
[[nodiscard]] Token operator+(Term lhs, Term rhs);
[[nodiscard]] Token operator+(Term lhs, Terms rhs);
[[nodiscard]] Token operator+(Term const &lhs, Expression rhs);

[[nodiscard]] Token operator-(Term lhs, Constant rhs);
[[nodiscard]] Token operator-(Term lhs, Variable rhs);
[[nodiscard]] Token operator-(Term const &lhs, Function rhs);
[[nodiscard]] Token operator-(Term lhs, Term const &rhs);
[[nodiscard]] Token operator-(Term lhs, Terms const &rhs);
[[nodiscard]] Token operator-(Term const &lhs, Expression rhs);

[[nodiscard]] Token operator*(Term lhs, Constant rhs);
[[nodiscard]] Token operator*(Term lhs, Variable rhs);
[[nodiscard]] Token operator*(Term lhs, Function const &rhs);
[[nodiscard]] Token operator*(Term lhs, Term const &rhs);
[[nodiscard]] Token operator*(Term const &lhs, Terms rhs);
[[nodiscard]] Token operator*(Term lhs, Expression const &rhs);

[[nodiscard]] Token operator/(Term lhs, Constant rhs);
[[nodiscard]] Token operator/(Term lhs, Variable rhs);
[[nodiscard]] Token operator/(Term lhs, Function const &rhs);
[[nodiscard]] Token operator/(Term lhs, Term const &rhs);
[[nodiscard]] Token operator/(Term const &lhs, Terms const &rhs);
[[nodiscard]] Token operator/(Term lhs, Expression const &rhs);

[[nodiscard]] Token pow(Term lhs, Constant rhs);
[[nodiscard]] Token pow(Term const &lhs, Variable rhs);
[[nodiscard]] Token pow(Term const &lhs, Function rhs);
[[nodiscard]] Token pow(Term const &lhs, Term rhs);
[[nodiscard]] Token pow(Term const &lhs, Terms rhs);
} // namespace mlp

#endif // TERM_H
