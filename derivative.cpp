#include "token.h"

#include <format>
#include <map>
#include <vector>

namespace {
std::map<std::string, std::string> k_function_map{
    {"sin", "cos({0})"},
    {"cos", "-sin({0})"},
    {"tan", "sec({0})^2"},
    {"sec", "sec({0})*tan({0})"},
    {"csc", "-csc({0})*cot({0})"},
    {"cot", "-csc({0})^2"},
    {"sinh", "cosh({0})"},
    {"cosh", "sinh({0})"},
    {"tanh", "sech({0})^2"},
    {"sech", "sech({0})*tanh({0})"},
    {"csch", "-csch({0})*cot({0})"},
    {"coth", "-csch({0})^2"},
    {"asin", "1/((1 - ({0})^2)^0.5)"},
    {"acos", "-1/((1 - ({0})^2)^0.5)"},
    {"atan", "1/(1 + ({0})^2)"},
    // {"asec", "1/(({0})*(({0})^2 - 1)^0.5"},
    // {"acsc", "-1/(({0})*(({0})^2 - 1)^0.5"},
    {"acot", "-1/(1 + ({0})^2)"},
    {"asinh", "1/((1 + ({0})^2)^0.5)"},
    {"acosh", "-1/((1 + ({0})^2)^0.5)"},
    {"atanh", "1/(1 - ({0})^2)"},
    {"asech", "-1/(({0})*(1 - ({0})^2)^0.5"},
    // {"acsch", "1/(({0})*(1 - ({0})^2)^0.5"},
    {"acoth", "1/(1 - ({0})^2)"},
    {"ln", "1/({0})"}
};
} // namespace

mlp::OwnedToken mlp::Differentiable::derivative(
    Variable const &variable, std::uint32_t const order,
    std::map<Variable, SharedToken> const &values
) const {
    return this->derivative(variable, order)->evaluate(values);
}

mlp::OwnedToken
mlp::Constant::derivative(Variable const &, std::uint32_t const) const {
    return std::make_unique<Constant>(0);
}

mlp::OwnedToken mlp::Variable::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return Owned<Variable>(this->clone());

    return std::make_unique<Constant>(*this == variable && order == 1 ? 1 : 0);
}

mlp::OwnedToken mlp::Function::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    Terms result{};

    auto parameter = static_cast<std::string>(*this->parameter);

    result.add_term(tokenise(std::vformat(
        k_function_map.at(this->function), std::make_format_args(parameter)
    )));

    result.add_term(this->parameter->derivative(variable, 1));

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Term::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    auto const &base_type = typeid(*this->base);

    double const c = this->coefficient;

    if (auto const &power_type = typeid(*this->power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_unique<Constant>(0);

        double const power = dynamic_cast<Constant &>(*this->power).value;

        Terms terms{};
        terms.add_term(std::make_unique<Term>(
            c * power, OwnedToken(this->base->clone()),
            std::make_unique<Constant>(power - 1)
        ));
        terms.add_term(this->base->derivative(variable, 1));

        auto derivative = terms.simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    Terms result{};

    result.add_term(OwnedToken(this->clone()));

    if (base_type == typeid(Constant)) {
        result.add_term(
            std::make_unique<Function>("ln", OwnedToken(this->base->clone()))
        );
        result.add_term(this->power->derivative(variable, 1));

        auto derivative = result.simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    auto terms_1 = std::make_unique<Terms>();
    terms_1->add_term(OwnedToken(this->power->clone()));
    terms_1->add_term(this->base->derivative(variable, 1));
    terms_1->add_term(std::make_unique<Term>(
        OwnedToken(this->base->clone()), std::make_unique<Constant>(-1)
    ));

    auto terms_2 = std::make_unique<Terms>();
    terms_2->add_term(this->power->derivative(variable, 1));
    terms_2->add_term(
        std::make_unique<Function>("ln", OwnedToken(this->base->clone()))
    );

    auto expression = std::make_unique<Expression>();
    expression->add_token(Sign::pos, std::move(terms_1));
    expression->add_token(Sign::pos, std::move(terms_2));

    result.add_term(std::move(expression));

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Terms::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (this->coefficient == 0 || !this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    auto result = std::make_unique<Expression>();

    for (int i = 0; i < this->terms.size(); ++i) {
        auto term = std::make_unique<Terms>();

        for (int j = 0; j < this->terms.size(); ++j) {
            if (i != j) {
                term->add_term(OwnedToken(this->terms[j]->clone()));

                continue;
            }

            auto derivative = this->terms[i]->derivative(variable, 1);

            if (typeid(*derivative) == typeid(Constant)) {
                term->coefficient *=
                    dynamic_cast<Constant &>(*derivative).value;

                continue;
            }

            term->add_term(std::move(derivative));
        }

        result->add_token(Sign::pos, std::move(term));
    }

    Term const end{
        this->coefficient, std::move(result), std::make_unique<Constant>(1)
    };

    auto derivative = end.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Expression::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    Expression result{};

    for (auto const &[operation, token] : this->tokens)
        result.add_token(operation, token->derivative(variable, 1));

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}