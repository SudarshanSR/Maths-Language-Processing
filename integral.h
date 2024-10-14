#ifndef INTEGRAL_H
#define INTEGRAL_H

#include <memory>

struct Token;
struct Variable;

using OwnedToken = std::unique_ptr<Token>;
using SharedToken = std::shared_ptr<Token>;

struct Integrable {
    virtual ~Integrable() = default;

    [[nodiscard]] virtual OwnedToken integral(Variable const &variable
    ) const = 0;

    [[nodiscard]] OwnedToken integral(
        Variable const &variable, SharedToken const &from, SharedToken const &to
    ) const;
};

#endif // INTEGRAL_H
