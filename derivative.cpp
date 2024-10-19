#include "derivative.h"
#include "dependent.h"
#include "evaluate.h"
#include "simplify.h"

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

mlp::OwnedToken mlp::derivative(
    Token const &token, Variable const &variable, std::uint32_t const order,
    std::map<Variable, SharedToken> const &values
) {
    return evaluate(*derivative(token, variable, order), values);
}

mlp::OwnedToken mlp::derivative(
    Token const &token, Variable const &variable, std::uint32_t const order
) {
    auto const &type = typeid(token);

    if (type == typeid(Constant))
        return std::make_unique<Constant>(
            derivative(dynamic_cast<Constant const &>(token), variable, order)
        );

    if (type == typeid(Variable)) {
        std::variant<Constant, Variable> variant =
            derivative(dynamic_cast<Variable const &>(token), variable, order);

        if (std::holds_alternative<Constant>(variant))
            return std::make_unique<Constant>(std::get<Constant>(variant));

        return std::make_unique<Variable>(std::get<Variable>(variant));
    }

    if (type == typeid(Function))
        return derivative(
            dynamic_cast<Function const &>(token), variable, order
        );

    if (type == typeid(Term))
        return derivative(dynamic_cast<Term const &>(token), variable, order);

    if (type == typeid(Terms))
        return derivative(dynamic_cast<Terms const &>(token), variable, order);

    if (type == typeid(Expression))
        return derivative(
            dynamic_cast<Expression const &>(token), variable, order
        );

    throw std::invalid_argument("Invalid argument!");
}

mlp::Constant
mlp::derivative(Constant const &, Variable const &, std::uint32_t) {
    return Constant(0);
}

std::variant<mlp::Constant, mlp::Variable> mlp::derivative(
    Variable const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    return Constant(token == variable && order == 1 ? 1 : 0);
}

mlp::OwnedToken mlp::derivative(
    Function const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token.clone();

    if (!is_dependent_on(token, variable))
        return std::make_unique<Constant>(0);

    Terms result{};

    auto parameter = static_cast<std::string>(*token.parameter);

    result.add_term(tokenise(
        std::vformat(
            k_function_map.at(token.function), std::make_format_args(parameter)
        )
    ));

    result.add_term(derivative(*token.parameter, variable, 1));

    auto derivative = simplified(result);

    if (order > 1)
        return simplified(*mlp::derivative(*derivative, variable, order - 1));

    return derivative;
}

mlp::OwnedToken mlp::derivative(
    Term const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token.clone();

    if (!is_dependent_on(token, variable))
        return std::make_unique<Constant>(0);

    auto const &base_type = typeid(*token.base);

    double const c = token.coefficient;

    if (auto const &power_type = typeid(*token.power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_unique<Constant>(0);

        double const power = dynamic_cast<Constant &>(*token.power).value;

        Terms terms{};
        terms.add_term(
            std::make_unique<Term>(
                c * power, token.base->clone(),
                std::make_unique<Constant>(power - 1)
            )
        );
        terms.add_term(derivative(*token.base, variable, 1));

        auto derivative = simplified(terms);

        if (order > 1)
            return simplified(*mlp::derivative(*derivative, variable, order - 1)
            );

        return derivative;
    }

    Terms result{};

    result.add_term(token.clone());

    if (base_type == typeid(Constant)) {
        result.add_term(std::make_unique<Function>("ln", token.base->clone()));
        result.add_term(derivative(*token.power, variable, 1));

        auto derivative = simplified(result);

        if (order > 1)
            return simplified(*mlp::derivative(*derivative, variable, order - 1)
            );

        return derivative;
    }

    auto terms_1 = std::make_unique<Terms>();
    terms_1->add_term(token.power->clone());
    terms_1->add_term(derivative(*token.base, variable, 1));
    terms_1->add_term(
        std::make_unique<Term>(
            token.base->clone(), std::make_unique<Constant>(-1)
        )
    );

    auto terms_2 = std::make_unique<Terms>();
    terms_2->add_term(derivative(*token.power, variable, 1));
    terms_2->add_term(std::make_unique<Function>("ln", token.base->clone()));

    auto expression = std::make_unique<Expression>();
    expression->add_token(Operation(Operation::add), std::move(terms_1));
    expression->add_token(Operation(Operation::add), std::move(terms_2));

    result.add_term(std::move(expression));

    auto derivative = simplified(result);

    if (order > 1)
        return simplified(*mlp::derivative(*derivative, variable, order - 1));

    return derivative;
}

mlp::OwnedToken mlp::derivative(
    Terms const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token.clone();

    if (token.coefficient == 0 || !is_dependent_on(token, variable))
        return std::make_unique<Constant>(0);

    auto result = std::make_unique<Expression>();

    for (int i = 0; i < token.terms.size(); ++i) {
        auto term = std::make_unique<Terms>();

        for (int j = 0; j < token.terms.size(); ++j) {
            if (i != j) {
                term->add_term(token.terms[j]->clone());

                continue;
            }

            auto derivative = mlp::derivative(*token.terms[i], variable, 1);

            if (typeid(*derivative) == typeid(Constant)) {
                term->coefficient *=
                    dynamic_cast<Constant &>(*derivative).value;

                continue;
            }

            term->add_term(std::move(derivative));
        }

        result->add_token(Operation{Operation::add}, std::move(term));
    }

    Term const end{
        token.coefficient, std::move(result), std::make_unique<Constant>(1)
    };

    auto derivative = simplified(end);

    if (order > 1)
        return simplified(*mlp::derivative(*derivative, variable, order - 1));

    return derivative;
}

mlp::OwnedToken mlp::derivative(
    Expression const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token.clone();

    if (!is_dependent_on(token, variable))
        return std::make_unique<Constant>(0);

    Expression result{};

    for (auto const &[operation, token] : token.tokens)
        result.add_token(operation, derivative(*token, variable, 1));

    auto derivative = simplified(result);

    if (order > 1)
        return simplified(*mlp::derivative(*derivative, variable, order - 1));

    return derivative;
}