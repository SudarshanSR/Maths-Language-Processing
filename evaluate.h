#ifndef EVALUATE_H
#define EVALUATE_H

#include "token.h"

#include <map>
#include <variant>

namespace mlp {
[[nodiscard]] OwnedToken
evaluate(Token const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] Constant
evaluate(Constant const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] OwnedToken
evaluate(Variable const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] std::variant<Constant, Function>
evaluate(Function const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] OwnedToken
evaluate(Term const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] OwnedToken
evaluate(Terms const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] OwnedToken evaluate(
    Expression const &token, std::map<Variable, SharedToken> const &values
);
} // namespace mlp

#endif // EVALUATE_H
