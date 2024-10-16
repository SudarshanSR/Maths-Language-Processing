#include "evaluate.h"
#include "simplify.h"

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
        return evaluate(dynamic_cast<Constant const &>(token), values);

    if (type == typeid(Variable))
        return evaluate(dynamic_cast<Variable const &>(token), values);

    if (type == typeid(Function))
        return evaluate(dynamic_cast<Function const &>(token), values);

    if (type == typeid(Term))
        return evaluate(dynamic_cast<Term const &>(token), values);

    if (type == typeid(Terms))
        return evaluate(dynamic_cast<Terms const &>(token), values);

    if (type == typeid(Expression))
        return evaluate(dynamic_cast<Expression const &>(token), values);

    throw std::invalid_argument("Invalid argument!");
}

mlp::OwnedToken
mlp::evaluate(Constant const &token, std::map<Variable, SharedToken> const &) {
    return token.clone();
}

mlp::OwnedToken mlp::evaluate(
    Variable const &token, std::map<Variable, SharedToken> const &values
) {
    return values.contains(token) ? values.at(token)->clone() : token.clone();
}

mlp::OwnedToken mlp::evaluate(
    Function const &token, std::map<Variable, SharedToken> const &values
) {
    auto param = evaluate(*token.parameter, values);

    if (typeid(*param) == typeid(Constant))
        return std::make_unique<Constant>(k_functions.at(token.function)(
            dynamic_cast<Constant &>(*param).value
        ));

    return std::make_unique<Function>(token.function, std::move(param));
}

mlp::OwnedToken mlp::evaluate(
    Term const &token, std::map<Variable, SharedToken> const &values
) {
    auto const clone = token.clone();
    auto &term = dynamic_cast<Term &>(*clone);
    term.base = evaluate(*term.base, values);
    term.power = evaluate(*term.power, values);

    return simplified(term);
}

mlp::OwnedToken mlp::evaluate(
    Terms const &token, std::map<Variable, SharedToken> const &values
) {
    auto const clone = token.clone();
    auto &terms = dynamic_cast<Terms &>(*clone);

    for (auto &term : terms.terms)
        term = evaluate(*term, values);

    return simplified(terms);
}

mlp::OwnedToken mlp::evaluate(
    Expression const &token, std::map<Variable, SharedToken> const &values
) {
    auto const clone = token.clone();
    auto &expression = dynamic_cast<Expression &>(*clone);

    for (auto &term : expression.tokens)
        if (typeid(*term) != typeid(Operation))
            term = evaluate(*term, values);

    return simplified(expression);
}