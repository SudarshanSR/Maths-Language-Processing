#ifndef DIFFERENTIATE_H
#define DIFFERENTIATE_H

#include "token.h"

std::shared_ptr<Token> differentiate(std::shared_ptr<Token> const &param,
                                     Variable const &variable,
                                     std::uint32_t order = 1);

#endif // DIFFERENTIATE_H
