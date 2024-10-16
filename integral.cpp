#include "integral.h"
#include "dependent.h"
#include "evaluate.h"
#include "simplify.h"

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

mlp::OwnedToken mlp::integral(
    Token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
) {
    auto const integral = mlp::integral(token, variable);

    Expression result{};
    result.add_token(evaluate(*integral, {{variable, to}}));
    result.add_token(std::make_unique<Operation>(Operation::sub));
    result.add_token(evaluate(*integral, {{variable, from}}));

    return simplified(result);
}

mlp::OwnedToken mlp::integral(Token const &token, Variable const &variable) {
    auto const &type = typeid(token);

    if (type == typeid(Constant))
        return integral(dynamic_cast<Constant const &>(token), variable);

    if (type == typeid(Variable))
        return integral(dynamic_cast<Variable const &>(token), variable);

    if (type == typeid(Function))
        return integral(dynamic_cast<Function const &>(token), variable);

    if (type == typeid(Term))
        return integral(dynamic_cast<Term const &>(token), variable);

    if (type == typeid(Terms))
        return integral(dynamic_cast<Terms const &>(token), variable);

    if (type == typeid(Expression))
        return integral(dynamic_cast<Expression const &>(token), variable);

    throw std::invalid_argument("Invalid argument!");
}

mlp::OwnedToken mlp::integral(Constant const &token, Variable const &variable) {
    return std::make_unique<Term>(
        token.value, variable.clone(), std::make_unique<Constant>(1)
    );
}

mlp::OwnedToken mlp::integral(Variable const &token, Variable const &variable) {
    if (token == variable)
        return std::make_unique<Term>(
            0.5, variable.clone(), std::make_unique<Constant>(2)
        );

    auto terms = std::make_unique<Terms>();
    terms->add_term(variable.clone());
    terms->add_term(token.clone());

    return terms;
}

mlp::OwnedToken mlp::integral(Function const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(variable.clone());
        terms->add_term(token.clone());

        return terms;
    }

    if (typeid(*token.parameter) == typeid(Variable)) {
        Terms terms{};

        auto string = static_cast<std::string>(*token.parameter);

        terms.add_term(tokenise(
            std::vformat(
                k_function_map.at(token.function), std::make_format_args(string)
            )
        ));

        return simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::integral(Term const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(variable.clone());
        terms->add_term(token.clone());

        return terms;
    }

    auto const &base_type = typeid(*token.base);
    auto const &power_type = typeid(*token.power);

    if (base_type == typeid(Variable) && power_type == typeid(Constant)) {
        auto const &power = dynamic_cast<Constant &>(*token.power);

        if (power.value == -1) {
            return std::make_unique<Term>(
                token.coefficient.value,
                std::make_unique<Function>("ln", token.base->clone()),
                std::make_unique<Constant>(1)
            );
        }

        return std::make_unique<Term>(
            token.coefficient.value / (power.value + 1), token.base->clone(),
            std::make_unique<Constant>(power.value + 1)
        );
    }

    if (base_type == typeid(Constant) && power_type == typeid(Variable)) {
        Terms terms{};
        terms.add_term(token.clone());

        if (dynamic_cast<Variable &>(*token.power) != variable) {
            terms.add_term(variable.clone());
        } else {
            terms.add_term(
                std::make_unique<Term>(
                    1, std::make_unique<Function>("ln", token.base->clone()),
                    std::make_unique<Constant>(-1)
                )
            );
        }

        return simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken mlp::integral(Terms const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(variable.clone());
        terms->add_term(token.clone());

        return terms;
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::OwnedToken
mlp::integral(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable)) {
        auto terms = std::make_unique<Terms>();
        terms->add_term(variable.clone());
        terms->add_term(token.clone());

        return terms;
    }

    Expression result{};

    for (OwnedToken const &term : token.tokens) {
        if (auto const &token_type = typeid(*term);
            token_type == typeid(Operation)) {
            result.add_token(term->clone());

            continue;
        }

        result.add_token(integral(*term, variable));
    }

    return simplified(result);
}
