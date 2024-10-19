#include "token.h"

#include <format>
#include <map>

namespace {
std::map<std::string, std::string> k_function_map{
    {"sin", "-cos({0})"},
    {"cos", "sin({0})"},
    // {"tan", "ln(sec({0}))"},
    // {"sec", "ln(sec({0}) + tan({0}))"},
    // {"csc", "ln(csc({0}) - cot({0}))"},
    // {"cot", "ln(sin({0}))"},
    {"sinh", "cosh({0})"},
    {"cosh", "sinh({0})"},
    // {"tanh", "ln(cosh({0}))"},
    {"sech", "atan(sinh({0}))"},
    {"csch", "acoth(cosh({0}))"},
    // {"coth", "ln(sinh({0}))"},
    {"ln", "{0}ln({0}) - {0}"},
    {"asin", "{0}asin({0}) + (1 - {0}^2)^0.5"},
    {"acos", "{0}acos({0}) - (1 - {0}^2)^0.5"},
    {"atan", "{0}atan({0}) - ln(1 + {0}^2) / 2"},
    {"asec", "{0}asec({0}) - ln({0} + ({0}^2 - 1)^0.5)"},
    {"acsc", "{0}acsc({0}) + ln({0} + ({0}^2 - 1)^0.5)"},
    {"acot", "{0}acot({0}) + ln(1 + {0}^2) / 2"},
};
} // namespace

mlp::OwnedToken mlp::Integrable::integral(
    Variable const &variable, SharedToken const &from, SharedToken const &to
) {
    auto const integral = this->integral(variable);

    Expression result{};
    result.add_token(Sign::pos, integral->evaluate({{variable, to}}));
    result.add_token(Sign::neg, integral->evaluate({{variable, from}}));

    return result.simplified();
}

mlp::OwnedToken mlp::Constant::integral(Variable const &variable) {
    return std::make_unique<Term>(
        this->value, Owned<Variable>(variable.clone()),
        std::make_unique<Constant>(1)
    );
}

mlp::OwnedToken mlp::Variable::integral(Variable const &variable) {
    if (*this == variable)
        return std::make_unique<Term>(
            0.5, Owned<Variable>(variable.clone()),
            std::make_unique<Constant>(2)
        );

    auto terms = std::make_unique<Terms>();
    terms->add_term(Owned<Variable>(variable.clone()));
    terms->add_term(Owned<Variable>(this->clone()));

    return terms;
}

mlp::OwnedToken mlp::Function::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(Owned<Variable>(variable.clone()));
        terms->add_term(Owned<Function>(this->clone()));

        return terms;
    }

    if (typeid(*this->parameter) == typeid(Variable)) {
        Terms terms{};

        auto string = static_cast<std::string>(*this->parameter);

        terms.add_term(tokenise(std::vformat(
            k_function_map.at(this->function), std::make_format_args(string)
        )));

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Term::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(Owned<Variable>(variable.clone()));
        terms->add_term(Owned<Term>(this->clone()));

        return terms;
    }

    auto const &base_type = typeid(*this->base);
    auto const &power_type = typeid(*this->power);

    if (base_type == typeid(Variable) && power_type == typeid(Constant)) {
        auto const &power = dynamic_cast<Constant &>(*this->power);

        if (power.value == -1) {
            return std::make_unique<Term>(
                this->coefficient,
                std::make_unique<Function>(
                    "ln", OwnedToken(this->base->clone())
                ),
                std::make_unique<Constant>(1)
            );
        }

        return std::make_unique<Term>(
            this->coefficient / (power.value + 1),
            OwnedToken(this->base->clone()),
            std::make_unique<Constant>(power.value + 1)
        );
    }

    if (base_type == typeid(Constant) && power_type == typeid(Variable)) {
        Terms terms{};
        terms.add_term(Owned<Term>(this->clone()));

        if (dynamic_cast<Variable &>(*this->power) != variable) {
            terms.add_term(Owned<Variable>(variable.clone()));
        } else {
            terms.add_term(std::make_unique<Term>(
                1,
                std::make_unique<Function>(
                    "ln", OwnedToken(this->base->clone())
                ),
                std::make_unique<Constant>(-1)
            ));
        }

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Terms::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(Owned<Variable>(variable.clone()));
        terms->add_term(Owned<Terms>(this->clone()));

        return terms;
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Expression::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(Owned<Variable>(variable.clone()));
        terms->add_term(Owned<Expression>(this->clone()));

        return terms;
    }

    Expression result{};

    for (auto const &[operation, term] : this->tokens)
        result.add_token(operation, term->integral(variable));

    return result.simplified();
}
