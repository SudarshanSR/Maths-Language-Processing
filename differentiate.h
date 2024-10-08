#ifndef DIFFERENTIATE_H
#define DIFFERENTIATE_H

#include "token.h"

std::shared_ptr<Token> differentiate(std::shared_ptr<Token> const &param,
                                     Variable const &variable,
                                     std::uint32_t order = 1);

std::shared_ptr<Token> differentiate(std::shared_ptr<Variable> const &param,
                                     Variable const &variable);

std::shared_ptr<Token> differentiate(std::shared_ptr<Term> const &term,
                                     Variable const &variable,
                                     std::uint32_t order = 1);

std::shared_ptr<Token> differentiate(std::shared_ptr<Function> const &function,
                                     Variable const &variable,
                                     std::uint32_t order = 1);

std::shared_ptr<Token>
differentiate(std::shared_ptr<Expression> const &expression,
              Variable const &variable, std::uint32_t order = 1);

#endif // DIFFERENTIATE_H
