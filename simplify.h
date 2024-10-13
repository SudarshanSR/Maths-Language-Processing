#ifndef SIMPIFY_H
#define SIMPIFY_H

#include <memory>

struct Token;

struct Simplifiable {
    virtual ~Simplifiable() = default;

    [[nodiscard]] virtual std::shared_ptr<Token> simplified() const = 0;
};

#endif // SIMPIFY_H
