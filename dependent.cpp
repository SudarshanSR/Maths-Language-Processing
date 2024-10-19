#include "token.h"

#include <algorithm>

bool mlp::Constant::is_dependent_on(Variable const &variable) const {
    return false;
}

bool mlp::Variable::is_dependent_on(Variable const &variable) const {
    return *this == variable;
}

bool mlp::Function::is_dependent_on(Variable const &variable) const {
    return this->parameter->is_dependent_on(variable);
}

bool mlp::Term::is_dependent_on(Variable const &variable) const {
    return this->base->is_dependent_on(variable) ||
           this->power->is_dependent_on(variable);
}

bool mlp::Terms::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->terms,
        [variable](OwnedToken const &term) -> bool {
            return term->is_dependent_on(variable);
        }
    );
}

bool mlp::Expression::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->tokens,
        [variable](std::pair<Sign, OwnedToken> const &term) -> bool {
            return term.second->is_dependent_on(variable);
        }
    );
}