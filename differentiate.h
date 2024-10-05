#ifndef DIFFERENTIATE_H
#define DIFFERENTIATE_H

#include "token.h"

Expression *differentiate(Term const &term, Variable const &variable);

Expression *differentiate(Function const &function, Variable const &variable);

Expression *differentiate(Expression const &expression,
                          Variable const &variable);

#endif // DIFFERENTIATE_H
