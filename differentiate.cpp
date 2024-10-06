#include "differentiate.h"

#include <map>

namespace {
std::map<std::string,
         std::vector<std::tuple<std::string, long double, long double>>>
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

Term get_next_term(std::vector<std::shared_ptr<Token>> const &tokens, int &i) {
    Term term{};

    std::shared_ptr<Token> token = tokens[i];

    while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
        if (typeid(*token) == typeid(Constant)) {
            if (term.coefficient)
                throw std::invalid_argument("Expression is not valid!");

            term.coefficient = std::dynamic_pointer_cast<Constant>(token);
        } else if (typeid(*token) == typeid(Variable) ||
                   typeid(*token) == typeid(Expression) ||
                   typeid(*token) == typeid(Function)) {
            if (term.base)
                throw std::invalid_argument("Expression is not valid!");

            term.base = token;
        }

        if (i == tokens.size() - 1)
            break;

        token = tokens[++i];
    }

    return term;
}

std::shared_ptr<Token> differentiate(std::shared_ptr<Token> const &param,
                                     Variable const &variable) {
    if (auto const expression = std::dynamic_pointer_cast<Expression>(param))
        return differentiate(*expression, variable);

    if (auto const function = std::dynamic_pointer_cast<Function>(param))
        return differentiate(*function, variable);

    if (auto const term = std::dynamic_pointer_cast<Term>(param))
        return differentiate(*term, variable);

    throw std::invalid_argument("Invalid argument!");
}
} // namespace

std::shared_ptr<Token> differentiate(Term const &term,
                                     Variable const &variable) {
    if (!term.base)
        return std::make_shared<Constant>(0);

    if (auto const var = std::dynamic_pointer_cast<Variable>(term.base);
        var && var->var != variable.var)
        return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Expression>();

    Constant c{1};

    if (term.coefficient)
        c = *term.coefficient;

    if (!term.power) {
        if (c.value != 1 || std::dynamic_pointer_cast<Variable>(term.base))
            result->add_token(std::make_shared<Constant>(c.value));

        if (typeid(*term.base) == typeid(Expression) ||
            typeid(*term.base) == typeid(Function) ||
            typeid(*term.base) == typeid(Term))
            result->add_token(differentiate(term.base, variable));
    } else {
        if (auto const p = std::dynamic_pointer_cast<Constant>(term.power)) {
            if (typeid(*term.base) == typeid(Constant))
                return std::make_shared<Constant>(0);

            long double const power = p->value;

            if (long double const coefficient = c.value * power;
                coefficient != 1 ||
                (power == 1 && std::dynamic_pointer_cast<Variable>(term.base)))
                result->add_token(std::make_shared<Constant>(c.value * power));

            if (power == 1) {
                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term))
                    result->add_token(differentiate(term.base, variable));

                return Expression::simplify(result);
            }

            result->add_token(term.base);

            if (power != 2) {
                result->add_token(std::make_shared<Operation>('^'));
                result->add_token(std::make_shared<Constant>(power - 1));
            }

            if (typeid(*term.base) == typeid(Expression) ||
                typeid(*term.base) == typeid(Function) ||
                typeid(*term.base) == typeid(Term)) {
                result->add_token(std::make_shared<Operation>('*'));
                result->add_token(differentiate(term.base, variable));
            }
        } else if (auto const power =
                       std::dynamic_pointer_cast<Variable>(term.power)) {
            if (power->var != variable.var) {
                if (typeid(*term.base) == typeid(Constant))
                    return std::make_shared<Constant>(0);

                auto const coefficient = std::make_shared<Expression>();

                if (c.value != 1)
                    coefficient->add_token(std::make_shared<Constant>(c));

                coefficient->add_token(power);

                result->add_token(coefficient);
                result->add_token(term.base);
                result->add_token(std::make_shared<Operation>('^'));

                auto const new_power = std::make_shared<Expression>();
                new_power->add_token(power);
                new_power->add_token(std::make_shared<Operation>('-'));
                new_power->add_token(std::make_shared<Constant>(1));

                result->add_token(new_power);

                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term)) {
                    result->add_token(std::make_shared<Operation>('*'));
                    result->add_token(differentiate(term.base, variable));
                }
            } else {
                if (c.value != 1)
                    result->add_token(std::make_shared<Constant>(c));

                auto const expression_1 = std::make_shared<Expression>();
                expression_1->add_token(term.base);
                expression_1->add_token(std::make_shared<Operation>('^'));
                expression_1->add_token(term.power);

                result->add_token(expression_1);
                result->add_token(std::make_shared<Operation>('*'));

                auto const expression_2 = std::make_shared<Expression>();
                result->add_token(expression_2);

                if (auto const base =
                        std::dynamic_pointer_cast<Constant>(term.base)) {
                    expression_2->add_token(std::make_shared<Function>(
                        "ln", std::make_shared<Constant>(1), base,
                        std::make_shared<Constant>(1)));

                    return Expression::simplify(result);
                }

                expression_2->add_token(term.power);

                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term)) {
                    expression_2->add_token(std::make_shared<Operation>('*'));
                    expression_2->add_token(differentiate(term.base, variable));
                } else if (auto const base =
                               std::dynamic_pointer_cast<Variable>(term.base)) {
                    if (base->var != variable.var) {
                        expression_2->add_token(
                            std::make_shared<Operation>('*'));

                        expression_2->add_token(base);
                    }
                }

                expression_2->add_token(std::make_shared<Operation>('/'));
                expression_2->add_token(term.base);

                expression_2->add_token(std::make_shared<Operation>('+'));

                expression_2->add_token(std::make_shared<Function>(
                    "ln", std::make_shared<Constant>(1), term.base,
                    std::make_shared<Constant>(1)));
            }
        } else {
            if (c.value != 1)
                result->add_token(std::make_shared<Constant>(c));

            auto const expression_1 = std::make_shared<Expression>();
            expression_1->add_token(term.base);
            expression_1->add_token(std::make_shared<Operation>('^'));
            expression_1->add_token(term.power);

            result->add_token(expression_1);
            result->add_token(std::make_shared<Operation>('*'));

            auto const expression_2 = std::make_shared<Expression>();
            result->add_token(expression_2);

            if (auto const base =
                    std::dynamic_pointer_cast<Constant>(term.base)) {
                expression_2->add_token(std::make_shared<Function>(
                    "ln", std::make_shared<Constant>(1), base,
                    std::make_shared<Constant>(1)));

                return Expression::simplify(result);
            }

            expression_2->add_token(term.power);

            if (typeid(*term.base) == typeid(Expression) ||
                typeid(*term.base) == typeid(Function) ||
                typeid(*term.base) == typeid(Term)) {
                expression_2->add_token(std::make_shared<Operation>('*'));
                expression_2->add_token(differentiate(term.base, variable));
            } else if (auto const base =
                           std::dynamic_pointer_cast<Variable>(term.base)) {
                if (base->var != variable.var) {
                    expression_2->add_token(std::make_shared<Operation>('*'));

                    expression_2->add_token(base);
                }
            }

            expression_2->add_token(std::make_shared<Operation>('/'));
            expression_2->add_token(term.base);

            expression_2->add_token(std::make_shared<Operation>('+'));

            if (typeid(*term.power) == typeid(Expression) ||
                typeid(*term.power) == typeid(Function) ||
                typeid(*term.power) == typeid(Term)) {
                result->add_token(std::make_shared<Operation>('*'));
                result->add_token(differentiate(term.power, variable));
            }

            expression_2->add_token(std::make_shared<Function>(
                "ln", std::make_shared<Constant>(1), term.base,
                std::make_shared<Constant>(1)));
        }
    }

    return Expression::simplify(result);
}

std::shared_ptr<Token> differentiate(Function const &function,
                                     Variable const &variable) {
    if (auto const p = std::dynamic_pointer_cast<Variable>(function.parameter))
        if (p->var != variable.var)
            return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Expression>();

    if (auto functions = k_function_map[function.function];
        functions.size() == 1) {
        auto const &[name, coefficient, power] = functions[0];

        result->add_token(std::make_shared<Function>(
            name,
            std::make_shared<Constant>(coefficient *
                                       function.coefficient->value),
            function.parameter, std::make_shared<Constant>(power)));
    } else {
        auto const expression = std::make_shared<Expression>();

        auto const product = function.coefficient;

        expression->add_token(product);

        for (int i = 0; i < functions.size() - 1; ++i) {
            auto const &[name, coefficient, power] = functions[i];

            product->value *= coefficient;

            expression->add_token(std::make_shared<Function>(
                name, std::make_shared<Constant>(1), function.parameter,
                std::make_shared<Constant>(power)));
            expression->add_token(std::make_shared<Operation>('*'));
        }

        auto const &[name, coefficient, power] = functions.back();

        product->value *= coefficient;

        expression->add_token(std::make_shared<Function>(
            name, std::make_shared<Constant>(1), function.parameter,
            std::make_shared<Constant>(power)));

        result->add_token(expression);
    }

    if (typeid(*function.parameter) != typeid(Variable)) {
        result->add_token(std::make_shared<Operation>('*'));

        if (typeid(*function.parameter) == typeid(Expression) ||
            typeid(*function.parameter) == typeid(Function) ||
            typeid(*function.parameter) == typeid(Term)) {
            result->add_token(std::make_shared<Operation>('*'));
            result->add_token(differentiate(function.parameter, variable));
        }
    }

    return Expression::simplify(result);
}

std::shared_ptr<Token> differentiate(Expression const &expression,
                                     Variable const &variable) {
    auto const result = std::make_shared<Expression>();

    std::vector<std::shared_ptr<Token>> const &tokens = expression.tokens();

    for (int i = 0; i < tokens.size(); ++i) {
        if (std::shared_ptr<Token> const &token = tokens[i];
            typeid(*token) == typeid(Operation)) {
            result->add_token(
                std::make_shared<Operation>(static_cast<std::string>(
                    *std::dynamic_pointer_cast<Operation>(token))[0]));

            continue;
        }

        Term term = get_next_term(tokens, i);

        if (i < tokens.size() - 1) {
            if (auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i])) {
                if (operation->operation == Operation::op::mul) {
                    Term right = get_next_term(tokens, ++i);

                    auto v = std::make_shared<Term>(right);

                    auto u_d = differentiate(term, variable);
                    auto v_d = differentiate(*v, variable);

                    result->add_token(std::make_shared<Term>(term));
                    result->add_token(std::make_shared<Operation>('*'));
                    result->add_token(v_d);

                    result->add_token(std::make_shared<Operation>('+'));

                    result->add_token(u_d);
                    result->add_token(std::make_shared<Operation>('*'));
                    result->add_token(v);

                    continue;
                }

                if (operation->operation == Operation::op::div) {
                    Term right = get_next_term(tokens, ++i);

                    auto v = std::make_shared<Term>(right);

                    auto u_d = differentiate(term, variable);
                    auto v_d = differentiate(*v, variable);

                    auto numerator = std::make_shared<Expression>();

                    numerator->add_token(u_d);
                    numerator->add_token(std::make_shared<Operation>('*'));
                    numerator->add_token(v);

                    numerator->add_token(std::make_shared<Operation>('-'));

                    numerator->add_token(std::make_shared<Term>(term));
                    numerator->add_token(std::make_shared<Operation>('*'));
                    numerator->add_token(v_d);

                    auto denominator = std::make_shared<Expression>();
                    denominator->add_token(v);
                    denominator->add_token(std::make_shared<Operation>('^'));
                    denominator->add_token(std::make_shared<Constant>(2));

                    result->add_token(numerator);
                    result->add_token(std::make_shared<Operation>('/'));
                    result->add_token(denominator);

                    continue;
                }

                if (operation->operation != Operation::op::pow) {
                    --i;

                    result->add_token(differentiate(term, variable));

                    continue;
                }

                try {
                    term.power = tokens.at(++i);
                } catch (std::out_of_range const &) {
                    throw std::invalid_argument("Expression is not valid!");
                }

                if (!term.base) {
                    if (term.coefficient) {
                        term.base = term.coefficient;
                        term.coefficient = nullptr;
                    } else {
                        term.base = std::make_shared<Constant>(1);
                    }
                }
            } else {
                --i;
            }
        }

        result->add_token(differentiate(term, variable));
    }

    return Expression::simplify(result);
}