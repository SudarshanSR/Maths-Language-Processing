#ifndef DEPENDENT_H
#define DEPENDENT_H

#include "token.h"

namespace mlp {
[[nodiscard]] bool
is_dependent_on(Token const &token, Variable const &variable);

[[nodiscard]] bool
is_dependent_on(Constant const &token, Variable const &variable);

[[nodiscard]] bool
is_dependent_on(Variable const &token, Variable const &variable);

[[nodiscard]] bool
is_dependent_on(Function const &token, Variable const &variable);

[[nodiscard]] bool is_dependent_on(Term const &token, Variable const &variable);

[[nodiscard]] bool
is_dependent_on(Terms const &token, Variable const &variable);

[[nodiscard]] bool
is_dependent_on(Expression const &token, Variable const &variable);
} // namespace mlp

#endif // DEPENDENT_H
