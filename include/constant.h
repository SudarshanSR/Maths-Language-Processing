#ifndef CONSTANT_H
#define CONSTANT_H

#include "token.h"

namespace mlp {
class Constant final : public Token {
    std::double_t value;

  public:
    using Token::derivative;
    using Token::integral;

    explicit Constant(std::double_t value);

    Constant(Constant const &) = default;

    Constant(Constant &&) = default;

    Constant &operator=(Constant const &) = default;

    Constant &operator=(Constant &&) = default;

    Constant &operator=(std::double_t value);

    [[nodiscard]] gsl::owner<Constant *> clone() const override;

    [[nodiscard]] gsl::owner<Constant *> move() && override;

    explicit operator std::string() const override;

    explicit(false) operator std::double_t() const;

    [[nodiscard]] Constant operator-() const;

    Constant &operator++();

    Constant &operator--();

    Constant &operator+=(std::double_t rhs);

    Constant &operator-=(std::double_t rhs);

    Constant &operator*=(std::double_t rhs);

    Constant &operator/=(std::double_t rhs);

    Constant &operator^=(std::double_t rhs);

    bool operator==(Constant const &) const;

    bool operator>(Constant const &) const;

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;
};

Constant operator++(Constant &lhs, int);

Constant operator--(Constant &lhs, int);

Constant operator+(Constant lhs, std::double_t rhs);

Constant operator+(std::double_t lhs, Constant rhs);

Constant operator-(Constant lhs, std::double_t rhs);

Constant operator-(std::double_t lhs, Constant rhs);

Constant operator*(Constant lhs, std::double_t rhs);

Constant operator*(std::double_t lhs, Constant rhs);

Constant operator/(Constant lhs, std::double_t rhs);

Constant operator/(std::double_t lhs, Constant rhs);

Constant operator^(Constant lhs, Constant const &rhs);

Constant operator^(Constant lhs, std::double_t rhs);

Constant operator^(std::double_t lhs, Constant rhs);
} // namespace mlp

#endif // CONSTANT_H
