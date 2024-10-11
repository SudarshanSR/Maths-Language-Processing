#include "differentiate.h"
#include "simplify.h"
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

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Variable> const &param, Variable const &variable,
    std::uint32_t const order = 1
) {
    if (!order)
        return param;

    return std::make_shared<Constant>(*param == variable && order == 1 ? 1 : 0);
}

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Term> const &term, Variable const &variable,
    std::uint32_t const order = 1
) {
    if (!order)
        return term;

    if (!term->base)
        return std::make_shared<Constant>(0);

    auto const &base_type = typeid(*term->base);

    Constant c = term->coefficient;

    if (!term->power) {
        return simplify(differentiate(
            simplify(std::make_shared<Term>(
                c.value, differentiate(term->base, variable),
                std::make_shared<Constant>(1)
            )),
            variable, order - 1
        ));
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
        terms->add_term(differentiate(term->base, variable));

        return simplify(differentiate(simplify(terms), variable, order - 1));
    }

    auto const result = std::make_shared<Terms>();

    result->add_term(term);

    if (base_type == typeid(Constant)) {
        auto const base = std::dynamic_pointer_cast<Constant>(term->base);

        result->add_term(std::make_shared<Function>("ln", base));
        result->add_term(differentiate(term->power, variable));

        return simplify(differentiate(simplify(result), variable, order - 1));
    }

    auto const terms_1 = std::make_shared<Terms>();
    terms_1->add_term(term->power);
    terms_1->add_term(differentiate(term->base, variable));
    terms_1->add_term(
        std::make_shared<Term>(term->base, std::make_shared<Constant>(-1))
    );

    auto const terms_2 = std::make_shared<Terms>();
    terms_2->add_term(differentiate(term->power, variable));
    terms_2->add_term(std::make_shared<Function>("ln", term->base));

    auto const expression = std::make_shared<Expression>();
    expression->add_token(terms_1);
    expression->add_token(std::make_shared<Operation>(Operation::add));
    expression->add_token(terms_2);

    result->add_term(expression);

    return simplify(differentiate(simplify(result), variable, order - 1));
}

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Function> const &function, Variable const &variable,
    std::uint32_t const order = 1
) {
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

    result->add_term(differentiate(function->parameter, variable));

    return simplify(differentiate(simplify(result), variable, order - 1));
}

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Expression> const &expression, Variable const &variable,
    std::uint32_t const order = 1
) {
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

        result->add_token(differentiate(term, variable));
    }

    return simplify(differentiate(simplify(result), variable, order - 1));
}

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Terms> const &terms, Variable const &variable,
    std::uint32_t const order = 1
) {
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

            if (auto derivative = differentiate(terms->terms[i], variable);
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

    return simplify(differentiate(simplify(end), variable, order - 1));
}
} // namespace

std::shared_ptr<Token> differentiate(
    std::shared_ptr<Token> const &param, Variable const &variable,
    std::uint32_t const order
) {
    if (!order)
        return param;

    if (typeid(*param) == typeid(Expression))
        return differentiate(
            std::dynamic_pointer_cast<Expression>(param), variable, order
        );

    if (typeid(*param) == typeid(Terms))
        return differentiate(
            std::dynamic_pointer_cast<Terms>(param), variable, order
        );

    if (typeid(*param) == typeid(Function))
        return differentiate(
            std::dynamic_pointer_cast<Function>(param), variable, order
        );

    if (typeid(*param) == typeid(Term))
        return differentiate(
            std::dynamic_pointer_cast<Term>(param), variable, order
        );

    if (typeid(*param) == typeid(Variable))
        return differentiate(
            std::dynamic_pointer_cast<Variable>(param), variable, order
        );

    if (typeid(*param) == typeid(Constant))
        return std::make_shared<Constant>(0);

    throw std::invalid_argument("Invalid argument!");
}