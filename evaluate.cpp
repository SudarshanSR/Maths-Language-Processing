#include "token.h"

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

mlp::OwnedToken
mlp::Constant::evaluate(std::map<Variable, SharedToken> const &values) const {
    return std::make_unique<Constant>(*this);
}

mlp::OwnedToken
mlp::Variable::evaluate(std::map<Variable, SharedToken> const &values) const {
    return OwnedToken(
        values.contains(*this) ? values.at(*this)->clone() : this->clone()
    );
}

mlp::OwnedToken
mlp::Function::evaluate(std::map<Variable, SharedToken> const &values) const {
    auto param = this->parameter->evaluate(values);

    if (typeid(*param) == typeid(Constant))
        return std::make_unique<Constant>(k_functions.at(this->function)(
            dynamic_cast<Constant &>(*param).value
        ));

    return std::make_unique<Function>(this->function, std::move(param));
}

mlp::OwnedToken
mlp::Term::evaluate(std::map<Variable, SharedToken> const &values) const {
    auto const term = Owned<Term>(this->clone());

    term->base = term->base->evaluate(values);
    term->power = term->power->evaluate(values);

    return term->simplified();
}

mlp::OwnedToken
mlp::Terms::evaluate(std::map<Variable, SharedToken> const &values) const {
    auto const terms = Owned<Terms>(this->clone());

    for (auto &term : terms->terms)
        term = term->evaluate(values);

    return terms->simplified();
}

mlp::OwnedToken
mlp::Expression::evaluate(std::map<Variable, SharedToken> const &values) const {
    auto const expression = Owned<Expression>(this->clone());

    for (auto &term : expression->tokens | std::views::values)
        term = term->evaluate(values);

    return expression->simplified();
}