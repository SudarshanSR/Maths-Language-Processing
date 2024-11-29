#ifndef TERMS_H
#define TERMS_H

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

    [[nodiscard]] Terms operator-() const;

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

    friend bool is_dependent_on(Terms const &token, Variable const &variable);

    friend bool is_linear_of(Terms const &token, Variable const &variable);

    friend token
    evaluate(Terms const &token, std::map<Variable, SharedToken> const &values);

    friend token simplified(Terms const &token);

    friend token derivative(
        Terms const &token, Variable const &variable, std::uint32_t order
    );

    friend token integral(Terms const &token, Variable const &variable);
};
} // namespace mlp

#endif // TERMS_H
