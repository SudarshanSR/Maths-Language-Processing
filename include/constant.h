#ifndef CONSTANT_H
#define CONSTANT_H

#include "token.h"

namespace mlp {
class Constant final : public Token {
    std::double_t value;

  public:
    explicit Constant(std::double_t value);

    Constant &operator=(std::double_t value);

    [[nodiscard]] gsl::owner<Constant *> clone() const override;

    explicit operator std::string() const override;

    explicit(false) operator std::double_t() const;

    Constant operator-() const;

    friend bool operator==(Constant const &lhs, Constant const &rhs);

    friend bool operator!=(Constant const &lhs, Constant const &rhs);

    friend bool operator>(Constant const &lhs, Constant const &rhs);

    friend bool operator>=(Constant const &lhs, Constant const &rhs);

    friend bool operator<(Constant const &lhs, Constant const &rhs);

    friend bool operator<=(Constant const &lhs, Constant const &rhs);

    friend Constant &operator++(Constant &lhs);

    friend Constant operator++(Constant &lhs, int);

    friend Constant &operator+=(Constant &lhs, Constant const &rhs);

    friend Constant &operator+=(Constant &lhs, std::double_t rhs);

    friend Constant operator+(Constant lhs, Constant const &rhs);

    friend Constant operator+(Constant lhs, std::double_t rhs);

    friend Constant operator+(std::double_t lhs, Constant rhs);

    friend Constant &operator--(Constant &lhs);

    friend Constant operator--(Constant &lhs, int);

    friend Constant &operator-=(Constant &lhs, Constant const &rhs);

    friend Constant &operator-=(Constant &lhs, std::double_t rhs);

    friend Constant operator-(Constant lhs, Constant const &rhs);

    friend Constant operator-(Constant lhs, std::double_t rhs);

    friend Constant operator-(std::double_t lhs, Constant rhs);

    friend Constant &operator*=(Constant &lhs, Constant const &rhs);

    friend Constant &operator*=(Constant &lhs, std::double_t rhs);

    friend Constant operator*(Constant lhs, Constant const &rhs);

    friend Constant operator*(Constant lhs, std::double_t rhs);

    friend Constant operator*(std::double_t lhs, Constant rhs);

    friend Constant &operator/=(Constant &lhs, Constant const &rhs);

    friend Constant &operator/=(Constant &lhs, std::double_t rhs);

    friend Constant operator/(Constant lhs, Constant const &rhs);

    friend Constant operator/(Constant lhs, std::double_t rhs);

    friend Constant operator/(std::double_t lhs, Constant rhs);

    friend Constant &operator^=(Constant &lhs, Constant const &rhs);

    friend Constant &operator^=(Constant &lhs, std::double_t rhs);

    friend Constant operator^(Constant lhs, Constant const &rhs);

    friend Constant operator^(Constant lhs, std::double_t rhs);

    friend Constant operator^(std::double_t lhs, Constant rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};
} // namespace mlp

#endif // CONSTANT_H
