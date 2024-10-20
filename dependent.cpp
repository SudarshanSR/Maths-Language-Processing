#include "token.h"

#include <algorithm>
#include <ranges>

bool mlp::Constant::is_dependent_on(Variable const &variable) const {
    return false;
}

bool mlp::Constant::is_linear_of(Variable const &variable) const {
    return false;
}

bool mlp::Variable::is_dependent_on(Variable const &variable) const {
    return *this == variable;
}

bool mlp::Variable::is_linear_of(Variable const &variable) const {
    return this->is_dependent_on(variable);
}

bool mlp::Function::is_dependent_on(Variable const &variable) const {
    return this->parameter->is_dependent_on(variable);
}

bool mlp::Function::is_linear_of(Variable const &variable) const {
    return false;
}

bool mlp::Term::is_dependent_on(Variable const &variable) const {
    return this->base->is_dependent_on(variable) ||
           this->power->is_dependent_on(variable);
}

bool mlp::Term::is_linear_of(Variable const &variable) const {
    return this->is_dependent_on(variable) &&
           typeid(*this->power) == typeid(Constant) &&
           dynamic_cast<Constant const &>(*this->power).value == 1 &&
           this->base->is_linear_of(variable);
}

bool mlp::Terms::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->terms,
        [variable](OwnedToken const &term) -> bool {
            return term->is_dependent_on(variable);
        }
    );
}

bool mlp::Terms::is_linear_of(Variable const &variable) const {
    if (!this->is_dependent_on(variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &token : this->terms) {
        if (!token->is_linear_of(variable))
            continue;

        if (is_linear)
            return false;

        is_linear = true;
    }

    return is_linear;
}

bool mlp::Expression::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->tokens,
        [variable](std::pair<Sign, OwnedToken> const &term) -> bool {
            return term.second->is_dependent_on(variable);
        }
    );
}

bool mlp::Expression::is_linear_of(Variable const &variable) const {
    if (!this->is_dependent_on(variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &token : this->tokens | std::views::values) {
        if (typeid(*token) == typeid(Constant))
            continue;

        if (!token->is_linear_of(variable))
            return false;

        is_linear = true;
    }

    return is_linear;
}