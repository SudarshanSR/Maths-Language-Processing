#include "dependent.h"

#include <algorithm>

bool mlp::is_dependent_on(Token const &token, Variable const &variable) {
    auto const &type = typeid(token);

    if (type == typeid(Constant))
        return is_dependent_on(dynamic_cast<Constant const &>(token), variable);

    if (type == typeid(Variable))
        return is_dependent_on(dynamic_cast<Variable const &>(token), variable);

    if (type == typeid(Function))
        return is_dependent_on(dynamic_cast<Function const &>(token), variable);

    if (type == typeid(Term))
        return is_dependent_on(dynamic_cast<Term const &>(token), variable);

    if (type == typeid(Terms))
        return is_dependent_on(dynamic_cast<Terms const &>(token), variable);

    if (type == typeid(Expression))
        return is_dependent_on(
            dynamic_cast<Expression const &>(token), variable
        );

    throw std::invalid_argument("Invalid argument!");
}

bool mlp::is_dependent_on(Constant const &, Variable const &) { return false; }

bool mlp::is_dependent_on(Variable const &token, Variable const &variable) {
    return token == variable;
}

bool mlp::is_dependent_on(Function const &token, Variable const &variable) {
    return is_dependent_on(*token.parameter, variable);
}

bool mlp::is_dependent_on(Term const &token, Variable const &variable) {
    return is_dependent_on(*token.base, variable) ||
           is_dependent_on(*token.power, variable);
}

bool mlp::is_dependent_on(Terms const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.terms,
        [variable](OwnedToken const &term) -> bool {
            return is_dependent_on(*term, variable);
        }
    );
}

bool mlp::is_dependent_on(Expression const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.tokens,
        [variable](auto const &term) -> bool {
            return is_dependent_on(*term.second, variable);
        }
    );
}