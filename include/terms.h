#ifndef TERMS_H
#define TERMS_H

#include "token.h"

#include <vector>

namespace mlp {
class Terms final {
    std::vector<OwnedToken> terms;

  public:
    std::double_t coefficient{1};

    Terms() = default;

    Terms(Terms const &);

    Terms(Terms &&) = default;

    Terms &operator=(Terms const &);

    Terms &operator=(Terms &&) = default;

    explicit operator std::string() const;

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

    friend Token
    evaluate(Terms const &token, std::map<Variable, Token> const &values);

    friend Token simplified(Terms const &token);

    friend Token derivative(
        Terms const &token, Variable const &variable, std::uint32_t order
    );

    friend Token integral(Terms const &token, Variable const &variable);
};
} // namespace mlp

#endif // TERMS_H
