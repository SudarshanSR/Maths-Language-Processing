#ifndef FUNCTION_H
#define FUNCTION_H

#include "token.h"

namespace mlp {
class Function final {
    std::string function;

    OwnedToken parameter;

  public:
    explicit Function(std::string function, OwnedToken &&parameter);

    Function(Function const &function);

    Function(Function &&function) = default;

    Function &operator=(Function const &function);

    Function &operator=(Function &&function) = default;

    explicit operator std::string() const;

    friend bool
    is_dependent_on(Function const &token, Variable const &variable);

    friend bool is_linear_of(Function const &token, Variable const &variable);

    friend Token
    evaluate(Function const &token, std::map<Variable, Token> const &values);

    friend Token simplified(Function const &token);

    friend Token derivative(
        Function const &token, Variable const &variable, std::uint32_t order
    );

    friend Token integral(Function const &token, Variable const &variable);
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