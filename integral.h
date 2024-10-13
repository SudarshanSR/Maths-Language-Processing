#ifndef INTEGRAL_H
#define INTEGRAL_H

#include <memory>

struct Token;
struct Variable;

struct Integrable {
    virtual ~Integrable() = default;

    [[nodiscard]] virtual std::shared_ptr<Token>
    integral(Variable const &variable) const = 0;

    [[nodiscard]] std::shared_ptr<Token> integral(
        Variable const &variable, std::shared_ptr<Token> const &from,
        std::shared_ptr<Token> const &to
    ) const;
};

#endif // INTEGRAL_H
