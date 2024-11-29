#ifndef CONSTANT_H
#define CONSTANT_H

#include "token.h"

namespace mlp {
class Constant final : public Token {
    std::double_t value_;

  public:
    explicit Constant(std::double_t value);

    Constant(Constant const &) = default;

    Constant(Constant &&) = default;

    Constant &operator=(Constant const &) = default;

    Constant &operator=(Constant &&) = default;

    Constant &operator=(std::double_t value);

    [[nodiscard]] std::double_t value() const { return this->value_; }

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
};

[[nodiscard]] bool is_dependent_on(Constant const &, Variable const &);

[[nodiscard]] bool is_linear_of(Constant const &, Variable const &);

[[nodiscard]] token
evaluate(Constant const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] token simplified(Constant const &token);

[[nodiscard]] token derivative(
    Constant const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] token integral(Constant const &token, Variable const &variable);

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
