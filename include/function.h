#ifndef FUNCTION_H
#define FUNCTION_H

#include "token.h"

namespace mlp {
class Function final : public Token {
    std::string function;

    OwnedToken parameter;

  public:
    explicit Function(std::string function, OwnedToken &&parameter);

    Function(Function const &function);

    Function(Function &&function) = default;

    [[nodiscard]] gsl::owner<Function *> clone() const override;

    [[nodiscard]] gsl::owner<Function *> move() && override;

    explicit operator std::string() const override;

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

#endif // FUNCTION_H
