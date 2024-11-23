#ifndef TERM_H
#define TERM_H

#include "token.h"

namespace mlp {
struct Term final : Token {
    std::double_t coefficient{1};
    OwnedToken base;
    OwnedToken power;

    using Token::derivative;
    using Token::integral;

    Term(std::double_t coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(std::double_t coefficient, Token &&base, Token &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    Term(Token &&base, Token &&power);

    Term(Term const &term);

    Term(Term &&) = default;

    [[nodiscard]] gsl::owner<Term *> clone() const override;

    [[nodiscard]] gsl::owner<Term *> move() && override;

    explicit operator std::string() const override;

    [[nodiscard]] Term operator-() const;

    friend Term operator-(Term const &rhs);

    Term &operator*=(std::double_t rhs);

    friend Term operator*(std::double_t lhs, Term rhs);

    friend Term operator*(Term lhs, std::double_t rhs);

    Term &operator/=(std::double_t rhs);

    friend Term operator/(std::double_t lhs, Term rhs);

    friend Term operator/(Term lhs, std::double_t rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;
};
} // namespace mlp

#endif // TERM_H
