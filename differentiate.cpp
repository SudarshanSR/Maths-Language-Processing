#include "token.h"

#include <map>
#include <vector>

namespace {
std::map<
    std::string, std::vector<std::tuple<std::string, long double, long double>>>
    k_function_map{
        {"sin", {{"cos", 1, 1}}},
        {"cos", {{"sin", -1, 1}}},
        {"tan", {{"sec", 1, 2}}},
        {"sec", {{"sec", 1, 1}, {"tan", 1, 1}}},
        {"csc", {{"csc", -1, 1}, {"cot", 1, 1}}},
        {"cot", {{"csc", -1, 2}}},
        {"sinh", {{"cosh", 1, 1}}},
        {"cosh", {{"sinh", 1, 1}}},
        {"tanh", {{"sech", 1, 2}}},
        {"sech", {{"sech", 1, 1}, {"tanh", 1, 1}}},
        {"csch", {{"csch", -1, 1}, {"coth", 1, 1}}},
        {"coth", {{"csch", -1, 2}}},
        {"ln", {{"", 1, -1}}},
        {"e^", {{"e^", 1, 1}}},
    };
} // namespace

std::shared_ptr<Token> Constant::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    return std::make_shared<Constant>(0);
}

std::shared_ptr<Token> Variable::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return std::make_shared<Variable>(this->var);

    return std::make_shared<Constant>(*this == variable && order == 1 ? 1 : 0);
}

std::shared_ptr<Token>
Operation::derivative(Variable const &variable, std::uint32_t order) const {
    throw std::runtime_error("Operator cannot be differentiated!");
}

std::shared_ptr<Token> Function::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    auto function = std::make_shared<Function>(*this);

    if (!order)
        return function;

    if (typeid(*function->parameter) == typeid(Variable))
        if (*std::dynamic_pointer_cast<Variable>(function->parameter) !=
            variable)
            return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Terms>();

    if (auto functions = k_function_map[function->function];
        functions.size() == 1) {
        auto const &[name, coefficient, power] = functions[0];

        result->add_term(std::make_shared<Term>(
            coefficient, std::make_shared<Function>(name, function->parameter),
            std::make_shared<Constant>(power)
        ));
    } else {
        for (auto const &[name, coefficient, power] : functions) {
            result->add_term(std::make_shared<Term>(
                coefficient,
                std::make_shared<Function>(name, function->parameter),
                std::make_shared<Constant>(power)
            ));
        }
    }

    result->add_term(function->parameter->derivative(variable, 1));

    auto derivative = result->simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

std::shared_ptr<Token>
Term::derivative(Variable const &variable, std::uint32_t const order) const {
    auto term = std::make_shared<Term>(*this);

    if (!order)
        return term;

    if (!term->base)
        return std::make_shared<Constant>(0);

    auto const &base_type = typeid(*term->base);

    Constant c = term->coefficient;

    if (!term->power) {
        auto derivative = std::make_shared<Term>(
                              c.value, term->base->derivative(variable, 1),
                              std::make_shared<Constant>(1)
        )
                              ->simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    if (auto const &power_type = typeid(*term->power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_shared<Constant>(0);

        long double const power =
            std::dynamic_pointer_cast<Constant>(term->power)->value;

        auto const terms = std::make_shared<Terms>();
        terms->coefficient.value *= c.value * power;
        terms->add_term(std::make_shared<Term>(
            1, term->base, std::make_shared<Constant>(power - 1)
        ));
        terms->add_term(term->base->derivative(variable, 1));

        auto derivative = terms->simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    auto const result = std::make_shared<Terms>();

    result->add_term(term);

    if (base_type == typeid(Constant)) {
        auto const base = std::dynamic_pointer_cast<Constant>(term->base);

        result->add_term(std::make_shared<Function>("ln", base));
        result->add_term(term->power->derivative(variable, 1));

        auto derivative = result->simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    auto const terms_1 = std::make_shared<Terms>();
    terms_1->add_term(term->power);
    terms_1->add_term(term->base->derivative(variable, 1));
    terms_1->add_term(
        std::make_shared<Term>(term->base, std::make_shared<Constant>(-1))
    );

    auto const terms_2 = std::make_shared<Terms>();
    terms_2->add_term(term->power->derivative(variable, 1));
    terms_2->add_term(std::make_shared<Function>("ln", term->base));

    auto const expression = std::make_shared<Expression>();
    expression->add_token(terms_1);
    expression->add_token(std::make_shared<Operation>(Operation::add));
    expression->add_token(terms_2);

    result->add_term(expression);

    auto derivative = result->simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

std::shared_ptr<Token>
Terms::derivative(Variable const &variable, std::uint32_t const order) const {
    auto terms = std::make_shared<Terms>(*this);

    if (!order)
        return terms;

    if (terms->coefficient.value == 0)
        return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Expression>();

    for (int i = 0; i < terms->terms.size(); ++i) {
        auto const term = std::make_shared<Terms>();

        for (int j = 0; j < terms->terms.size(); ++j) {
            if (i != j) {
                term->add_term(terms->terms[j]);

                continue;
            }

            if (auto derivative = terms->terms[i]->derivative(variable, 1);
                typeid(*derivative) == typeid(Constant)) {
                term->coefficient.value *=
                    std::dynamic_pointer_cast<Constant>(derivative)->value;
            } else if (typeid(*derivative) == typeid(Term)) {
                auto t = std::dynamic_pointer_cast<Term>(derivative);
                term->coefficient.value *= t->coefficient.value;
                t->coefficient.value = 1;

                term->add_term(t);
            } else if (typeid(*derivative) == typeid(Terms)) {
                auto t = std::dynamic_pointer_cast<Terms>(derivative);
                term->coefficient.value *= t->coefficient.value;
                t->coefficient.value = 1;

                term->add_term(t);
            } else {
                term->add_term(derivative);
            }
        }

        result->add_token(term);

        if (i != terms->terms.size() - 1) {
            result->add_token(std::make_shared<Operation>(Operation::add));
        }
    }

    auto const end = std::make_shared<Terms>();
    end->coefficient = terms->coefficient;
    end->add_term(result);

    auto derivative = end->simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

std::shared_ptr<Token> Expression::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    auto expression = std::make_shared<Expression>(*this);

    if (!order)
        return expression;

    auto const result = std::make_shared<Expression>();

    for (std::shared_ptr<Token> const &term : expression->tokens) {
        if (auto const &token_type = typeid(*term);
            token_type == typeid(Operation)) {
            result->add_token(std::make_shared<Operation>(
                std::dynamic_pointer_cast<Operation>(term)->operation
            ));

            continue;
        }

        result->add_token(term->derivative(variable, 1));
    }

    auto derivative = result->simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}