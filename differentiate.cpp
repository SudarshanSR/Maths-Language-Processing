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

std::shared_ptr<Term>
get_next_term(std::vector<std::shared_ptr<Token>> const &tokens, int &i) {
    std::shared_ptr<Token> token = tokens[i];

    if (typeid(*token) == typeid(Term)) {
        ++i;

        return std::dynamic_pointer_cast<Term>(token);
    }

    auto term = std::make_shared<Term>();

    while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
        if (typeid(*token) == typeid(Constant)) {
            if (!term->coefficient)
                term->coefficient = std::make_shared<Constant>(1);

            term->coefficient->value *=
                std::dynamic_pointer_cast<Constant>(token)->value;
        } else if (typeid(*token) == typeid(Variable) ||
                   typeid(*token) == typeid(Expression) ||
                   typeid(*token) == typeid(Function)) {
            if (term->base)
                throw std::invalid_argument("Expression is not valid!");

            term->base = token;
        }

        if (i == tokens.size() - 1)
            break;

        token = tokens[++i];
    }

    return term;
}
} // namespace

std::shared_ptr<Token> differentiate(std::shared_ptr<Token> const &param,
                                     Variable const &variable,
                                     std::uint32_t const order) {
    if (!order)
        return param;

    if (typeid(*param) == typeid(Expression))
        return differentiate(std::dynamic_pointer_cast<Expression>(param),
                             variable, order);

    if (typeid(*param) == typeid(Function))
        return differentiate(std::dynamic_pointer_cast<Function>(param),
                             variable, order);

    if (typeid(*param) == typeid(Term))
        return differentiate(std::dynamic_pointer_cast<Term>(param), variable,
                             order);

    if (typeid(*param) == typeid(Variable))
        return differentiate(std::dynamic_pointer_cast<Variable>(param),
                             variable);

    throw std::invalid_argument("Invalid argument!");
}

std::shared_ptr<Token> differentiate(std::shared_ptr<Variable> const &param,
                                     Variable const &variable) {
    return std::make_shared<Constant>(*param == variable ? 1 : 0);
}

std::shared_ptr<Token> differentiate(std::shared_ptr<Term> const &term,
                                     Variable const &variable,
                                     std::uint32_t const order) {
    if (!order)
        return term;

    if (!term->base)
        return std::make_shared<Constant>(0);

    auto const &base_type = typeid(*term->base);

    if (base_type == typeid(Variable) &&
        *std::dynamic_pointer_cast<Variable>(term->base) != variable)
        return std::make_shared<Constant>(0);

    Constant c{1};

    if (term->coefficient)
        c = *term->coefficient;

    if (!term->power) {
        return simplify(differentiate(
            simplify(std::make_shared<Term>(std::make_shared<Constant>(c.value),
                                            differentiate(term->base, variable),
                                            std::make_shared<Constant>(1))),
            variable, order - 1));
    }

    auto const result = std::make_shared<Expression>();

    auto const &power_type = typeid(*term->power);

    if (power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_shared<Constant>(0);

        long double const power =
            std::dynamic_pointer_cast<Constant>(term->power)->value;

        result->add_token(std::make_shared<Term>(
            std::make_shared<Constant>(c.value * power), term->base,
            std::make_shared<Constant>(power - 1)));
        result->add_token(std::make_shared<Operation>('*'));
        result->add_token(differentiate(term->base, variable));

        return simplify(differentiate(simplify(result), variable, order - 1));
    }

    result->add_token(term);
    result->add_token(std::make_shared<Operation>('*'));

    auto const expression_2 = std::make_shared<Expression>();
    result->add_token(expression_2);

    if (base_type == typeid(Constant)) {
        auto const base = std::dynamic_pointer_cast<Constant>(term->base);

        expression_2->add_token(
            std::make_shared<Term>(std::make_shared<Constant>(1),
                                   std::make_shared<Function>("ln", base),
                                   std::make_shared<Constant>(1)));
        expression_2->add_token(std::make_unique<Operation>('*'));
        expression_2->add_token(differentiate(term->power, variable));

        return simplify(differentiate(simplify(result), variable, order - 1));
    }

    expression_2->add_token(term->power);
    expression_2->add_token(std::make_shared<Operation>('*'));
    expression_2->add_token(differentiate(term->base, variable));
    expression_2->add_token(std::make_shared<Operation>('/'));
    expression_2->add_token(term->base);

    expression_2->add_token(std::make_shared<Operation>('+'));

    expression_2->add_token(differentiate(term->power, variable));
    expression_2->add_token(std::make_shared<Operation>('*'));
    expression_2->add_token(
        std::make_shared<Term>(std::make_shared<Constant>(1),
                               std::make_shared<Function>("ln", term->base),
                               std::make_shared<Constant>(1)));

    return simplify(differentiate(simplify(result), variable, order - 1));
}

std::shared_ptr<Token> differentiate(std::shared_ptr<Function> const &function,
                                     Variable const &variable,
                                     std::uint32_t const order) {
    if (!order)
        return function;

    if (typeid(*function->parameter) == typeid(Variable))
        if (*std::dynamic_pointer_cast<Variable>(function->parameter) !=
            variable)
            return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Expression>();

    if (auto functions = k_function_map[function->function];
        functions.size() == 1) {
        auto const &[name, coefficient, power] = functions[0];

        result->add_token(std::make_shared<Term>(
            std::make_shared<Constant>(coefficient),
            std::make_shared<Function>(name, function->parameter),
            std::make_shared<Constant>(power)));
    } else {
        auto const expression = std::make_shared<Expression>();

        auto const product = std::make_shared<Constant>(1);

        expression->add_token(product);

        for (auto const &[name, coefficient, power] : functions) {
            expression->add_token(std::make_shared<Operation>('*'));

            product->value *= coefficient;

            expression->add_token(std::make_shared<Term>(
                std::make_shared<Constant>(1),
                std::make_shared<Function>(name, function->parameter),
                std::make_shared<Constant>(power)));
        }

        result->add_token(expression);
    }

    result->add_token(std::make_shared<Operation>('*'));
    result->add_token(differentiate(function->parameter, variable));

    return simplify(differentiate(simplify(result), variable, order - 1));
}

std::shared_ptr<Token>
differentiate(std::shared_ptr<Expression> const &expression,
              Variable const &variable, std::uint32_t const order) {
    if (!order)
        return expression;

    auto const result = std::make_shared<Expression>();

    std::vector<std::shared_ptr<Token>> const &tokens = expression->tokens;

    for (int i = 0; i < tokens.size(); ++i) {
        std::shared_ptr<Token> const &token = tokens[i];

        auto const &token_type = typeid(*token);

        if (token_type == typeid(Operation)) {
            result->add_token(
                std::make_shared<Operation>(static_cast<std::string>(
                    *std::dynamic_pointer_cast<Operation>(token))[0]));

            continue;
        }

        std::shared_ptr<Term> term = get_next_term(tokens, i);

        if (i >= tokens.size() - 1) {
            result->add_token(differentiate(term, variable));

            continue;
        }

        if (typeid(*tokens[i]) != typeid(Operation)) {
            --i;

            result->add_token(differentiate(term, variable));

            continue;
        }

        auto const operation = std::dynamic_pointer_cast<Operation>(tokens[i]);

        if (operation->operation == Operation::op::mul) {
            std::shared_ptr<Term> v = get_next_term(tokens, ++i);

            auto u_d = differentiate(term, variable);
            auto v_d = differentiate(v, variable);

            result->add_token(term);
            result->add_token(std::make_shared<Operation>('*'));
            result->add_token(v_d);

            result->add_token(std::make_shared<Operation>('+'));

            result->add_token(u_d);
            result->add_token(std::make_shared<Operation>('*'));
            result->add_token(v);

            continue;
        }

        if (operation->operation == Operation::op::div) {
            std::shared_ptr<Term> v = get_next_term(tokens, ++i);

            auto u_d = differentiate(term, variable);
            auto v_d = differentiate(v, variable);

            auto numerator = std::make_shared<Expression>();

            numerator->add_token(u_d);
            numerator->add_token(std::make_shared<Operation>('*'));
            numerator->add_token(v);

            numerator->add_token(std::make_shared<Operation>('-'));

            numerator->add_token(term);
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
            term->power = tokens.at(++i);
        } catch (std::out_of_range const &) {
            throw std::invalid_argument("Expression is not valid!");
        }

        if (!term->base) {
            if (term->coefficient) {
                term->base = term->coefficient;
                term->coefficient = nullptr;
            } else {
                term->base = std::make_shared<Constant>(1);
            }
        }

        result->add_token(differentiate(term, variable));
    }

    return simplify(differentiate(simplify(result), variable, order - 1));
}