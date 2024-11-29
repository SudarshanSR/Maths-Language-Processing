#ifndef TERM_H
#define TERM_H

#include "token.h"

namespace mlp {
struct Term final {
    std::double_t coefficient{1};
    OwnedToken base;
    OwnedToken power;

    Term(std::double_t coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(std::double_t coefficient, Token &&base, Token &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    Term(Token &&base, Token &&power);

    Term(Term const &term);

    Term(Term &&) = default;

    Term &operator=(Term const &term);

    Term &operator=(Term &&) = default;

    explicit operator std::string() const;

    [[nodiscard]] Term operator-() const;

    friend Term operator-(Term const &rhs);

    Term &operator*=(std::double_t rhs);

    friend Term operator*(std::double_t lhs, Term rhs);

    friend Term operator*(Term lhs, std::double_t rhs);

    Term &operator/=(std::double_t rhs);

    friend Term operator/(std::double_t lhs, Term rhs);

    friend Term operator/(Term lhs, std::double_t rhs);
};

[[nodiscard]] bool is_dependent_on(Term const &token, Variable const &variable);

[[nodiscard]] bool is_linear_of(Term const &token, Variable const &variable);

[[nodiscard]] Token
evaluate(Term const &token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Term const &token);

[[nodiscard]] Token
derivative(Term const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] Token integral(Term const &token, Variable const &variable);
} // namespace mlp

#endif // TERM_H
