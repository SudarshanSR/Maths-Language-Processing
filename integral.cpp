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

    if (this->parameter->is_linear_of(variable)) {
        Terms terms{};

        auto string = static_cast<std::string>(*this->parameter);

        terms.add_term(tokenise(std::vformat(
            k_function_map.at(this->function), std::make_format_args(string)
        )));
        terms.add_term(Term(
                           1, this->parameter->derivative(variable, 1),
                           std::make_unique<Constant>(-1)
        )
                           .simplified());

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

    if (power_type == typeid(Constant) && this->base->is_linear_of(variable)) {
        auto const &power = dynamic_cast<Constant &>(*this->power);
        Terms terms{};

        if (power.value == -1)
            terms.add_term(std::make_unique<Term>(
                this->coefficient,
                std::make_unique<Function>(
                    "ln", OwnedToken(this->base->clone())
                ),
                std::make_unique<Constant>(1)
            ));

        else
            terms.add_term(std::make_unique<Term>(
                this->coefficient / (power.value + 1),
                OwnedToken(this->base->clone()),
                std::make_unique<Constant>(power.value + 1)
            ));

        terms.add_term(Term(
                           1, this->base->derivative(variable, 1),
                           std::make_unique<Constant>(-1)
        )
                           .simplified());

        return terms.simplified();
    }

    if (base_type == typeid(Constant) && this->power->is_linear_of(variable)) {
        Terms terms{};
        terms.add_term(Owned<Term>(this->clone()));

        if (power_type == typeid(Variable) &&
            dynamic_cast<Variable &>(*this->power) != variable)
            terms.add_term(Owned<Variable>(variable.clone()));

        else if (power_type != typeid(Variable))
            terms.add_term(Term(
                               1, this->power->derivative(variable, 1),
                               std::make_unique<Constant>(-1)
            )
                               .simplified());

        terms.add_term(std::make_unique<Term>(
            1,
            std::make_unique<Function>("ln", OwnedToken(this->base->clone())),
            std::make_unique<Constant>(-1)
        ));

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

    if (this->is_linear_of(variable)) {
        Terms terms{};
        terms.coefficient = this->coefficient;

        for (OwnedToken const &token : this->terms)
            terms.add_term(
                token->is_linear_of(variable) ? token->integral(variable)
                                              : Owned<Token>(token->clone())
            );

        return terms.simplified();
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
