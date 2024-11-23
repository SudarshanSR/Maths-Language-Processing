module;

#include "token.h"

export module mlp.function;

namespace mlp {
export class Function final : public Token {
    std::string function;

    OwnedToken parameter;

  public:
    using Token::derivative;
    using Token::integral;

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

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;
};

class FunctionFactory final {
    std::string function;

  public:
    explicit FunctionFactory(std::string function);

    Function operator()(Token const &token) const;
};
} // namespace mlp

export mlp::FunctionFactory operator""_f(char const *string, size_t);
