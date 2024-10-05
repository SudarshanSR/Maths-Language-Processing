#ifndef DIFFERENTIATE_H
#define DIFFERENTIATE_H

#include "token.h"

Token *differentiate(Term const &term, Variable const &variable);

Token *differentiate(Function const &function, Variable const &variable);

Token *differentiate(Expression const &expression, Variable const &variable);

#endif // DIFFERENTIATE_H
