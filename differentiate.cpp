#include "differentiate.h"

namespace {
Term get_next_term(std::vector<Token *> const &tokens, int &i) {
    Term term{};

    Token *token = tokens[i];

    while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
        if (typeid(*token) == typeid(Constant)) {
            if (term.constant)
                throw std::invalid_argument("Expression is not valid!");

            term.constant = dynamic_cast<Constant *>(token);
        } else if (typeid(*token) == typeid(Variable) ||
                   typeid(*token) == typeid(Expression)) {
            if (term.base)
                throw std::invalid_argument("Expression is not valid!");

            term.base = token;
        }

        token = tokens[++i];
    }

    if (i < tokens.size())
        if (auto const *operation = dynamic_cast<Operation *>(token);
            operation && operation->operation == Operation::op::pow) {
            try {
                token = tokens.at(++i);
            } catch (std::out_of_range const &) {
                throw std::invalid_argument("Expression is not valid!");
            }

            term.power = token;
        }

    return term;
}
} // namespace

Expression *differentiate(Term const &term, Variable const &variable) {
    auto *result = new Expression;

    if (!term.base) {
        result->add_token(new Constant(0));

        return result;
    }

    if (auto const *var = dynamic_cast<Variable *>(term.base);
        var && var->var != variable.var) {
        result->add_token(new Constant(0));

        return result;
    }

    Constant c{1};

    if (term.constant)
        c = *term.constant;

    if (!term.power) {
        if (c.value != 1)
            result->add_token(new Constant(c.value));

        if (typeid(*term.base) == typeid(Expression))
            result->add_token(differentiate(
                *dynamic_cast<Expression *>(term.base), variable));
    } else {
        if (typeid(*term.power) == typeid(Constant)) {
            long double const power =
                dynamic_cast<Constant *>(term.power)->value;

            long double const coefficient = c.value * power;

            if (coefficient != 1)
                result->add_token(new Constant(c.value * power));

            if (power == 1) {
                if (typeid(*term.base) == typeid(Expression))
                    result->add_token(differentiate(
                        *dynamic_cast<Expression *>(term.base), variable));

                return result;
            }

            if (typeid(*term.base) == typeid(Variable))
                result->add_token(
                    new Variable(dynamic_cast<Variable *>(term.base)->var));

            else
                result->add_token(
                    new Expression(*dynamic_cast<Expression *>(term.base)));

            if (power != 2) {
                result->add_token(new Operation('^'));
                result->add_token(new Constant(power - 1));
            }

            if (typeid(*term.base) == typeid(Expression)) {
                result->add_token(new Operation('*'));
                result->add_token(differentiate(
                    *dynamic_cast<Expression *>(term.base), variable));
            }
        } else if (typeid(*term.power) == typeid(Variable)) {
            if (auto const *power = dynamic_cast<Variable *>(term.power);
                power->var != variable.var) {
                // TODO -> MAKE VAR A CONST
            } else {
                // TODO -> CHAIN POWER RULE
            }
        } else {
            // TODO -> CHAIN POWER RULE
            // result->add_token(new Constant(constant.value));
            // result->add_token(new Operation('*'));
            // result->add_token(
            //     new Expression(differentiate(*term.power, variable)));
            // result->add_token(new Operation('*'));
            // result->add_token(new Variable(term.variable->var));
        }
    }

    return result;
}

Expression *differentiate(Expression const &expression,
                          Variable const &variable) {
    auto *result = new Expression;

    std::vector<Token *> const &tokens = expression.tokens();

    for (int i = 0; i < tokens.size(); ++i) {
        if (Token *token = tokens[i]; typeid(*token) == typeid(Operation)) {
            result->add_token(new Operation(static_cast<std::string>(
                *dynamic_cast<Operation *>(token))[0]));

            continue;
        }

        Term const &term = get_next_term(tokens, i);

        if (i < tokens.size()) {
            if (auto const *operation = dynamic_cast<Operation *>(tokens[i])) {
                if (i == tokens.size() - 1)
                    throw std::invalid_argument("Expression is not valid!");

                if (operation->operation == Operation::op::mul) {
                    auto *v = new Expression(get_next_term(tokens, ++i));
                    auto *u = new Expression(term);

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    result->add_token(u);
                    result->add_token(new Operation('*'));
                    result->add_token(v_d);

                    result->add_token(new Operation('+'));

                    result->add_token(u_d);
                    result->add_token(new Operation('*'));
                    result->add_token(v);
                } else if (operation->operation == Operation::op::div) {
                    auto *v = new Expression(get_next_term(tokens, ++i));
                    auto *u = new Expression(term);

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    auto *numerator = new Expression;

                    numerator->add_token(u_d);
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v);

                    numerator->add_token(new Operation('-'));

                    numerator->add_token(u);
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v_d);

                    auto *denominator = new Expression;
                    denominator->add_token(v);
                    denominator->add_token(new Operation('^'));
                    denominator->add_token(new Constant(2));

                    result->add_token(numerator);
                    result->add_token(new Operation('/'));
                    result->add_token(denominator);
                } else {
                    --i;
                }

                continue;
            }

            --i;
        }

        result->add_token(differentiate(term, variable));
    }

    return result;
}