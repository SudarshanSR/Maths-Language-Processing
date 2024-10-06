#ifndef DIFFERENTIATE_H
#define DIFFERENTIATE_H

#include "token.h"

std::shared_ptr<Token> differentiate(Term const &term,
                                     Variable const &variable);

std::shared_ptr<Token> differentiate(Function const &function,
                                     Variable const &variable);

std::shared_ptr<Token> differentiate(Expression const &expression,
                                     Variable const &variable);

#endif // DIFFERENTIATE_H
