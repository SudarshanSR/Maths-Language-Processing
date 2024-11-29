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

    friend bool
    is_dependent_on(Function const &token, Variable const &variable);

    friend bool is_linear_of(Function const &token, Variable const &variable);

    friend token evaluate(
        Function const &token, std::map<Variable, SharedToken> const &values
    );

    friend token simplified(Function const &token);

    friend token derivative(
        Function const &token, Variable const &variable, std::uint32_t order
    );

    friend token integral(Function const &token, Variable const &variable);
};

class FunctionFactory final {
    std::string function;

  public:
    explicit FunctionFactory(std::string function);

    Function operator()(Token const &token) const;
};
} // namespace mlp

mlp::FunctionFactory operator""_f(char const *string, size_t);

#endif //
