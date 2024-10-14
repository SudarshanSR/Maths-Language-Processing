#ifndef SIMPIFY_H
#define SIMPIFY_H

#include <memory>

struct Token;

using OwnedToken = std::unique_ptr<Token>;

struct Simplifiable {
    virtual ~Simplifiable() = default;

    [[nodiscard]] virtual OwnedToken simplified() const = 0;
};

#endif // SIMPIFY_H
