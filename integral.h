#ifndef INTEGRAL_H
#define INTEGRAL_H

#include "token.h"

#include <memory>

namespace mlp {
[[nodiscard]] OwnedToken integral(
    Token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
);

[[nodiscard]] OwnedToken integral(Token const &token, Variable const &variable);

[[nodiscard]] Term integral(Constant const &token, Variable const &variable);

[[nodiscard]] OwnedToken
integral(Variable const &token, Variable const &variable);

[[nodiscard]] OwnedToken
integral(Function const &token, Variable const &variable);

[[nodiscard]] OwnedToken integral(Term const &token, Variable const &variable);

[[nodiscard]] OwnedToken integral(Terms const &token, Variable const &variable);

[[nodiscard]] OwnedToken
integral(Expression const &token, Variable const &variable);
} // namespace mlp

#endif // INTEGRAL_H
