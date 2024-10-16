#ifndef SIMPLIFY_H
#define SIMPLIFY_H

#include "token.h"

#include <memory>

namespace mlp {
[[nodiscard]] OwnedToken simplified(Token const &token);

[[nodiscard]] OwnedToken simplified(Constant const &token);

[[nodiscard]] OwnedToken simplified(Variable const &token);

[[nodiscard]] OwnedToken simplified(Function const &token);

[[nodiscard]] OwnedToken simplified(Term const &token);

[[nodiscard]] OwnedToken simplified(Terms const &token);

[[nodiscard]] OwnedToken simplified(Expression const &token);
} // namespace mlp

#endif // SIMPLIFY_H