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
    result += integral->evaluate({{variable, to}});
    result -= integral->evaluate({{variable, from}});

    return result.simplified();
}

mlp::OwnedToken mlp::Constant::integral(Variable const &variable) {
    return std::make_unique<Term>(*this * (variable ^ 1));
}

mlp::OwnedToken mlp::Variable::integral(Variable const &variable) {
    if (*this == variable)
        return std::make_unique<Term>((variable ^ 2) / 2);

    auto terms = std::make_unique<Terms>();
    *terms *= Owned<Variable>(variable.clone());
    *terms *= Owned<Variable>(this->clone());

    return terms;
}

mlp::OwnedToken mlp::Function::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Owned<Variable>(variable.clone());
        *terms *= Owned<Function>(this->clone());

        return terms;
    }

    if (this->parameter->is_linear_of(variable)) {
        Terms terms{};

        auto string = static_cast<std::string>(*this->parameter);

        terms *= tokenise(
            std::vformat(
                k_function_map.at(this->function), std::make_format_args(string)
            )
        );
        terms /= this->parameter->derivative(variable, 1)->simplified();

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Term::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Owned<Variable>(variable.clone());
        *terms *= Owned<Term>(this->clone());

        return terms;
    }

    auto const &power_type = typeid(*this->power);

    if (power_type == typeid(Constant) && this->base->is_linear_of(variable)) {
        auto const &power = dynamic_cast<Constant const &>(*this->power);
        Terms terms{};
        terms *= this->coefficient;

        if (power == -1) {
            terms *= std::make_unique<Function>(
                "ln", OwnedToken(this->base->clone())
            );
        } else {
            terms /= power + 1;
            terms *= OwnedToken(this->base->clone());
        }

        terms /= this->base->derivative(variable, 1)->simplified();

        return terms.simplified();
    }

    if (typeid(*this->base) == typeid(Constant) &&
        this->power->is_linear_of(variable)) {
        Terms terms{};
        terms *= Owned<Term>(this->clone());

        if (power_type == typeid(Variable) &&
            dynamic_cast<Variable const &>(*this->power) != variable)
            terms *= Owned<Variable>(variable.clone());

        else if (power_type != typeid(Variable))
            terms /= this->power->derivative(variable, 1)->simplified();

        terms /=
            std::make_unique<Function>("ln", OwnedToken(this->base->clone()));

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Terms::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Owned<Variable>(variable.clone());
        *terms *= Owned<Terms>(this->clone());

        return terms;
    }

    if (this->is_linear_of(variable)) {
        Terms terms{};
        terms.coefficient = this->coefficient;

        for (OwnedToken const &token : this->terms)
            terms *= token->is_linear_of(variable)
                         ? token->integral(variable)
                         : Owned<Token>(token->clone());

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::Expression::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Owned<Variable>(variable.clone());
        *terms *= Owned<Expression>(this->clone());

        return terms;
    }

    Expression result{};

    for (auto const &[operation, term] : this->tokens)
        result.add_token(operation, term->integral(variable));

    return result.simplified();
}
