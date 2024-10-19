#include "evaluate.h"
#include "simplify.h"

#include <cmath>
#include <map>
#include <ranges>

namespace {
std::map<std::string, double (*)(double)> k_functions{
    {"sin", std::sin},
    {"cos", std::cos},
    {"tan", std::tan},
    {"sec", [](double const val) -> double { return 1 / std::cos(val); }},
    {"csc", [](double const val) -> double { return 1 / std::sin(val); }},
    {"cot", [](double const val) -> double { return 1 / std::tan(val); }},
    {"sinh", std::sinh},
    {"cosh", std::cosh},
    {"tanh", std::tanh},
    {"sech", [](double const val) -> double { return 1 / std::cosh(val); }},
    {"csch", [](double const val) -> double { return 1 / std::sinh(val); }},
    {"coth", [](double const val) -> double { return 1 / std::tanh(val); }},
    {"asin", std::asin},
    {"acos", std::acos},
    {"atan", std::atan},
    {"asec", [](double const val) -> double { return std::acos(1 / val); }},
    {"acsc", [](double const val) -> double { return std::asin(1 / val); }},
    {"acot", [](double const val) -> double { return std::atan(1 / val); }},
    {"asinh", std::asinh},
    {"acosh", std::acosh},
    {"atanh", std::atanh},
    {"asech", [](double const val) -> double { return std::acosh(1 / val); }},
    {"acsch", [](double const val) -> double { return std::asinh(1 / val); }},
    {"acoth", [](double const val) -> double { return std::atanh(1 / val); }},
    {"ln", std::log},
};
} // namespace

mlp::OwnedToken mlp::evaluate(
    Token const &token, std::map<Variable, SharedToken> const &values
) {
    auto const &type = typeid(token);

    if (type == typeid(Constant))
        return std::make_unique<Constant>(
            evaluate(dynamic_cast<Constant const &>(token), values)
        );

    if (type == typeid(Variable))
        return evaluate(dynamic_cast<Variable const &>(token), values);

    if (type == typeid(Function)) {
        auto variant = evaluate(dynamic_cast<Function const &>(token), values);

        if (std::holds_alternative<Constant>(variant))
            return std::make_unique<Constant>(std::get<Constant>(variant));

        return std::make_unique<Function>(std::get<Function>(variant));
    }

    if (type == typeid(Term))
        return evaluate(dynamic_cast<Term const &>(token), values);

    if (type == typeid(Terms))
        return evaluate(dynamic_cast<Terms const &>(token), values);

    if (type == typeid(Expression))
        return evaluate(dynamic_cast<Expression const &>(token), values);

    throw std::invalid_argument("Invalid argument!");
}

mlp::Constant
mlp::evaluate(Constant const &token, std::map<Variable, SharedToken> const &) {
    return token;
}

mlp::OwnedToken mlp::evaluate(
    Variable const &token, std::map<Variable, SharedToken> const &values
) {
    return OwnedToken(
        values.contains(token) ? values.at(token)->clone() : token.clone()
    );
}

std::variant<mlp::Constant, mlp::Function> mlp::evaluate(
    Function const &token, std::map<Variable, SharedToken> const &values
) {
    auto param = evaluate(*token.parameter, values);

    if (typeid(*param) == typeid(Constant))
        return Constant(k_functions.at(token.function)(
            dynamic_cast<Constant &>(*param).value
        ));

    return Function(token.function, std::move(param));
}

mlp::OwnedToken mlp::evaluate(
    Term const &token, std::map<Variable, SharedToken> const &values
) {
    auto const term = Owned<Term>(token.clone());

    term->base = evaluate(*term->base, values);
    term->power = evaluate(*term->power, values);

    return simplified(*term);
}

mlp::OwnedToken mlp::evaluate(
    Terms const &token, std::map<Variable, SharedToken> const &values
) {
    auto const terms = Owned<Terms>(token.clone());

    for (auto &term : terms->terms)
        term = evaluate(*term, values);

    return simplified(*terms);
}

mlp::OwnedToken mlp::evaluate(
    Expression const &token, std::map<Variable, SharedToken> const &values
) {
    auto const expression = Owned<Expression>(token.clone());

    for (auto &term : expression->tokens | std::views::values)
        term = evaluate(*term, values);

    return simplified(*expression);
}