#ifndef TERMS_H
#define TERMS_H

#include "constant.h"
#include "token.h"

#include <vector>

namespace mlp {
class Terms final : public Token {
    std::vector<OwnedToken> terms;

  public:
    std::double_t coefficient{1};

    Terms() = default;

    Terms(Terms const &);

    Terms(Terms &&) = default;

    [[nodiscard]] gsl::owner<Terms *> clone() const override;

    [[nodiscard]] gsl::owner<Terms *> move() && override;

    explicit operator std::string() const override;

    Terms operator-() const;

    Terms &operator*=(Token const &token);

    Terms &operator/=(Token const &token);

    Terms &operator*=(OwnedToken &&token);

    Terms &operator/=(OwnedToken &&token);

    Terms &operator*=(Token &&token);

    Terms &operator/=(Token &&token);

    Terms &operator*=(std::double_t scalar);

    friend Terms operator*(std::double_t lhs, Terms rhs);

    friend Terms operator*(Terms lhs, std::double_t rhs);

    Terms &operator/=(std::double_t scalar);

    friend Terms operator/(std::double_t lhs, Terms rhs);

    friend Terms operator/(Terms lhs, std::double_t rhs);

    Terms &operator*=(Constant const &rhs);

    friend Terms operator*(Terms lhs, Constant const &rhs);

    Terms &operator/=(Constant const &rhs);

    friend Terms operator/(Terms lhs, Constant const &rhs);

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

#endif // TERMS_H
