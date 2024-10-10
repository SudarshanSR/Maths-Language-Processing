#ifndef SIMPLIFY_H
#define SIMPLIFY_H

#include <memory>

struct Token;

std::shared_ptr<Token> simplify(std::shared_ptr<Token> const &);

#endif // SIMPLIFY_H
