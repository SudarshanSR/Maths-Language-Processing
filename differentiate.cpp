#include "differentiate.h"

#include <map>
#include <memory>

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

Term get_next_term(std::vector<Token *> const &tokens, int &i) {
    Term term{};

    Token *token = tokens[i];

    while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
        if (typeid(*token) == typeid(Constant)) {
            if (term.coefficient)
                throw std::invalid_argument("Expression is not valid!");

            term.coefficient = dynamic_cast<Constant *>(copy(token));
        } else if (typeid(*token) == typeid(Variable) ||
                   typeid(*token) == typeid(Expression) ||
                   typeid(*token) == typeid(Function)) {
            if (term.base)
                throw std::invalid_argument("Expression is not valid!");

            term.base = copy(token);
        }

        token = tokens[++i];
    }

    return term;
}

Token *differentiate(Token const *param, Variable const &variable) {
    if (auto *expression = dynamic_cast<Expression const *>(param))
        return differentiate(*expression, variable);

    if (auto *function = dynamic_cast<Function const *>(param))
        return differentiate(*function, variable);

    if (auto *term = dynamic_cast<Term const *>(param))
        return differentiate(*term, variable);

    throw std::invalid_argument("Invalid argument!");
}
} // namespace

Token *differentiate(Term const &term, Variable const &variable) {
    if (!term.base)
        return new Constant(0);

    if (auto const *var = dynamic_cast<Variable *>(term.base);
        var && var->var != variable.var)
        return new Constant(0);

    auto result = std::make_unique<Expression>();

    Constant c{1};

    if (term.coefficient)
        c = *term.coefficient;

    if (!term.power) {
        if (c.value != 1 || dynamic_cast<Variable *>(term.base))
            result->add_token(new Constant(c.value));

        if (typeid(*term.base) == typeid(Expression) ||
            typeid(*term.base) == typeid(Function) ||
            typeid(*term.base) == typeid(Term))
            result->add_token(differentiate(term.base, variable));
    } else {
        if (auto const *p = dynamic_cast<Constant *>(term.power)) {
            if (typeid(*term.base) == typeid(Constant))
                return new Constant(0);

            long double const power = p->value;

            if (long double const coefficient = c.value * power;
                coefficient != 1 ||
                (power == 1 && dynamic_cast<Variable *>(term.base)))
                result->add_token(new Constant(c.value * power));

            if (power == 1) {
                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term))
                    result->add_token(differentiate(term.base, variable));

                return Expression::simplify(result.release());
            }

            result->add_token(copy(term.base));

            if (power != 2) {
                result->add_token(new Operation('^'));
                result->add_token(new Constant(power - 1));
            }

            if (typeid(*term.base) == typeid(Expression) ||
                typeid(*term.base) == typeid(Function) ||
                typeid(*term.base) == typeid(Term)) {
                result->add_token(new Operation('*'));
                result->add_token(differentiate(term.base, variable));
            }
        } else if (auto const *power = dynamic_cast<Variable *>(term.power)) {
            if (power->var != variable.var) {
                if (typeid(*term.base) == typeid(Constant))
                    return new Constant(0);

                auto *coefficient = new Expression;

                if (c.value != 1)
                    coefficient->add_token(copy(&c));

                coefficient->add_token(copy(power));

                result->add_token(coefficient);

                result->add_token(copy(term.base));

                result->add_token(new Operation('^'));

                auto *new_power = new Expression;
                new_power->add_token(new Variable(power->var));
                new_power->add_token(new Operation('-'));
                new_power->add_token(new Constant(1));

                result->add_token(new_power);

                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term)) {
                    result->add_token(new Operation('*'));
                    result->add_token(differentiate(term.base, variable));
                }
            } else {
                if (c.value != 1)
                    result->add_token(copy(&c));

                auto *expression_1 = new Expression;
                expression_1->add_token(copy(term.base));
                expression_1->add_token(new Operation('^'));
                expression_1->add_token(copy(term.power));

                result->add_token(expression_1);
                result->add_token(new Operation('*'));

                auto *expression_2 = new Expression;
                result->add_token(expression_2);

                if (auto const *base = dynamic_cast<Constant *>(term.base)) {
                    expression_2->add_token(new Function(
                        "ln", new Constant(1), copy(base), new Constant(1)));

                    Token *token = Expression::simplify(result.release());

                    return token;
                }

                expression_2->add_token(copy(term.power));

                if (typeid(*term.base) == typeid(Expression) ||
                    typeid(*term.base) == typeid(Function) ||
                    typeid(*term.base) == typeid(Term)) {
                    expression_2->add_token(new Operation('*'));
                    expression_2->add_token(differentiate(term.base, variable));
                } else if (auto const *base =
                               dynamic_cast<Variable *>(term.base)) {
                    if (base->var != variable.var) {
                        expression_2->add_token(new Operation('*'));

                        expression_2->add_token(copy(base));
                    }
                }

                expression_2->add_token(new Operation('/'));
                expression_2->add_token(copy(term.base));

                expression_2->add_token(new Operation('+'));

                expression_2->add_token(new Function(
                    "ln", new Constant(1), copy(term.base), new Constant(1)));
            }
        } else {
            if (c.value != 1)
                result->add_token(copy(&c));

            auto *expression_1 = new Expression;
            expression_1->add_token(copy(term.base));
            expression_1->add_token(new Operation('^'));
            expression_1->add_token(copy(term.power));

            result->add_token(expression_1);
            result->add_token(new Operation('*'));

            auto *expression_2 = new Expression;
            result->add_token(expression_2);

            if (auto const *base = dynamic_cast<Constant *>(term.base)) {
                expression_2->add_token(new Function(
                    "ln", new Constant(1), copy(base), new Constant(1)));

                Token *token = Expression::simplify(result.release());

                return token;
            }

            expression_2->add_token(copy(term.power));

            if (typeid(*term.base) == typeid(Expression) ||
                typeid(*term.base) == typeid(Function) ||
                typeid(*term.base) == typeid(Term)) {
                expression_2->add_token(new Operation('*'));
                expression_2->add_token(differentiate(term.base, variable));
            } else if (auto const *base = dynamic_cast<Variable *>(term.base)) {
                if (base->var != variable.var) {
                    expression_2->add_token(new Operation('*'));

                    expression_2->add_token(copy(base));
                }
            }

            expression_2->add_token(new Operation('/'));
            expression_2->add_token(copy(term.base));

            expression_2->add_token(new Operation('+'));

            if (typeid(*term.power) == typeid(Expression) ||
                typeid(*term.power) == typeid(Function) ||
                typeid(*term.power) == typeid(Term)) {
                result->add_token(new Operation('*'));
                result->add_token(differentiate(term.power, variable));
            }

            expression_2->add_token(new Function(
                "ln", new Constant(1), copy(term.base), new Constant(1)));
        }
    }

    Token *token = Expression::simplify(result.release());

    return token;
}

Token *differentiate(Function const &function, Variable const &variable) {
    if (auto const *p = dynamic_cast<Variable *>(function.parameter))
        if (p->var != variable.var)
            return new Constant(0);

    auto result = std::make_unique<Expression>();

    if (auto functions = k_function_map[function.function];
        functions.size() == 1) {
        auto const &[name, coefficient, power] = functions[0];

        result->add_token(new Function(
            name, new Constant(coefficient * function.coefficient->value),
            function.parameter, new Constant(power)));
    } else {
        auto *expression = new Expression;

        auto *product = dynamic_cast<Constant *>(copy(function.coefficient));

        expression->add_token(product);

        for (int i = 0; i < functions.size() - 1; ++i) {
            auto const &[name, coefficient, power] = functions[i];

            product->value *= coefficient;

            expression->add_token(new Function(name, new Constant(1),
                                               function.parameter,
                                               new Constant(power)));
            expression->add_token(new Operation('*'));
        }

        auto const &[name, coefficient, power] = functions.back();

        product->value *= coefficient;

        expression->add_token(new Function(
            name, new Constant(1), function.parameter, new Constant(power)));

        result->add_token(expression);
    }

    if (typeid(*function.parameter) != typeid(Variable)) {
        result->add_token(new Operation('*'));

        if (typeid(*function.parameter) == typeid(Expression) ||
            typeid(*function.parameter) == typeid(Function) ||
            typeid(*function.parameter) == typeid(Term)) {
            result->add_token(new Operation('*'));
            result->add_token(differentiate(function.parameter, variable));
        }
    }

    return Expression::simplify(result.release());
}

Token *differentiate(Expression const &expression, Variable const &variable) {
    auto result = std::make_unique<Expression>();

    std::vector<Token *> const &tokens = expression.tokens();

    for (int i = 0; i < tokens.size(); ++i) {
        if (Token *token = tokens[i]; typeid(*token) == typeid(Operation)) {
            result->add_token(new Operation(static_cast<std::string>(
                *dynamic_cast<Operation *>(token))[0]));

            continue;
        }

        Term term = get_next_term(tokens, i);

        if (i < tokens.size() - 1) {
            if (auto const *operation = dynamic_cast<Operation *>(tokens[i])) {
                if (operation->operation == Operation::op::mul) {
                    Term right = get_next_term(tokens, ++i);

                    auto *v = dynamic_cast<Term *>(copy(&right));

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    result->add_token(copy(&term));
                    result->add_token(new Operation('*'));
                    result->add_token(v_d);

                    result->add_token(new Operation('+'));

                    result->add_token(u_d);
                    result->add_token(new Operation('*'));
                    result->add_token(v);

                    continue;
                }

                if (operation->operation == Operation::op::div) {
                    Term right = get_next_term(tokens, ++i);

                    auto *v = dynamic_cast<Term *>(copy(&right));

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    auto *numerator = new Expression;

                    numerator->add_token(u_d);
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v);

                    numerator->add_token(new Operation('-'));

                    numerator->add_token(copy(&term));
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v_d);

                    auto *denominator = new Expression;
                    denominator->add_token(copy(v));
                    denominator->add_token(new Operation('^'));
                    denominator->add_token(new Constant(2));

                    result->add_token(numerator);
                    result->add_token(new Operation('/'));
                    result->add_token(denominator);

                    continue;
                }

                if (operation->operation != Operation::op::pow) {
                    --i;

                    result->add_token(differentiate(term, variable));

                    continue;
                }

                try {
                    term.power = copy(tokens.at(++i));
                } catch (std::out_of_range const &) {
                    throw std::invalid_argument("Expression is not valid!");
                }

                if (!term.base) {
                    if (term.coefficient) {
                        term.base = copy(term.coefficient);
                        term.coefficient = nullptr;
                    } else {
                        term.base = new Constant(1);
                    }
                }
            } else {
                --i;
            }
        }

        result->add_token(differentiate(term, variable));
    }

    Token *token = Expression::simplify(result.release());

    return token;
}