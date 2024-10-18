#ifndef DERIVATIVE_H
#define DERIVATIVE_H

#include "token.h"

#include <map>
#include <memory>
#include <variant>

namespace mlp {
[[nodiscard]] OwnedToken derivative(
    Token const &token, Variable const &variable, std::uint32_t order,
    std::map<Variable, SharedToken> const &values
);

[[nodiscard]] OwnedToken
derivative(Token const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] Constant derivative(
    Constant const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] std::variant<Constant, Variable> derivative(
    Variable const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] OwnedToken derivative(
    Function const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] OwnedToken
derivative(Term const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] OwnedToken
derivative(Terms const &token, Variable const &variable, std::uint32_t order);

[[nodiscard]] OwnedToken derivative(
    Expression const &token, Variable const &variable, std::uint32_t order
);
} // namespace mlp

#endif // DERIVATIVE_H