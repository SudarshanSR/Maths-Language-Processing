#include "token.h"

#include <map>
#include <vector>

namespace {
std::map<std::string, std::string> k_function_map{
    {"sin", "cos({0})"},
    {"cos", "-sin({0})"},
    {"tan", "sec({0})^2"},
    {"sec", "sec({0})*tan({0})"},
    {"csc", "-csc({0})*cot({0})"},
    {"cot", "-csc({0})^2"},
    {"sinh", "cosh({0})"},
    {"cosh", "sinh({0})"},
    {"tanh", "sech({0})^2"},
    {"sech", "sech({0})*tanh({0})"},
    {"csch", "-csch({0})*cot({0})"},
    {"coth", "-csch({0})^2"},
    {"asin", "1/((1 - ({0})^2)^0.5)"},
    {"acos", "-1/((1 - ({0})^2)^0.5)"},
    {"atan", "1/(1 + ({0})^2)"},
    // {"asec", "1/(({0})*(({0})^2 - 1)^0.5"},
    // {"acsc", "-1/(({0})*(({0})^2 - 1)^0.5"},
    {"acot", "-1/(1 + ({0})^2)"},
    {"asinh", "1/((1 + ({0})^2)^0.5)"},
    {"acosh", "-1/((1 + ({0})^2)^0.5)"},
    {"atanh", "1/(1 - ({0})^2)"},
    {"asech", "-1/(({0})*(1 - ({0})^2)^0.5"},
    // {"acsch", "1/(({0})*(1 - ({0})^2)^0.5"},
    {"acoth", "1/(1 - ({0})^2)"},
    {"ln", "1/({0})"}
};
} // namespace

std::shared_ptr<Token> Differentiable::derivative(
    Variable const &variable, std::uint32_t const order,
    std::map<Variable, std::shared_ptr<Token>> const &values
) const {
    return std::dynamic_pointer_cast<Evaluatable>(
               this->derivative(variable, order)
    )
        ->at(values);
}

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

std::shared_ptr<Token> Function::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return std::make_shared<Function>(*this);

    if (typeid(*this->parameter) == typeid(Variable))
        if (*std::dynamic_pointer_cast<Variable>(this->parameter) != variable)
            return std::make_shared<Constant>(0);

    auto const result = std::make_shared<Terms>();

    auto parameter = static_cast<std::string>(*this->parameter);

    result->add_term(tokenise(std::vformat(
        k_function_map.at(this->function), std::make_format_args(parameter)
    )));

    result->add_term(std::dynamic_pointer_cast<Differentiable>(this->parameter)
                         ->derivative(variable, 1));

    auto derivative = result->simplified();

    if (order > 1)
        return std::dynamic_pointer_cast<Simplifiable>(
                   std::dynamic_pointer_cast<Differentiable>(derivative)
                       ->derivative(variable, order - 1)
        )
            ->simplified();

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
        auto derivative =
            std::make_shared<Term>(
                c.value,
                std::dynamic_pointer_cast<Differentiable>(term->base)
                    ->derivative(variable, 1),
                std::make_shared<Constant>(1)
            )
                ->simplified();

        if (order > 1)
            return std::dynamic_pointer_cast<Simplifiable>(
                       std::dynamic_pointer_cast<Differentiable>(derivative)
                           ->derivative(variable, order - 1)
            )
                ->simplified();

        return derivative;
    }

    if (auto const &power_type = typeid(*term->power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_shared<Constant>(0);

        double const power =
            std::dynamic_pointer_cast<Constant>(term->power)->value;

        auto const terms = std::make_shared<Terms>();
        terms->coefficient.value *= c.value * power;
        terms->add_term(std::make_shared<Term>(
            1, term->base, std::make_shared<Constant>(power - 1)
        ));
        terms->add_term(std::dynamic_pointer_cast<Differentiable>(term->base)
                            ->derivative(variable, 1));

        auto derivative = terms->simplified();

        if (order > 1)
            return std::dynamic_pointer_cast<Simplifiable>(
                       std::dynamic_pointer_cast<Differentiable>(derivative)
                           ->derivative(variable, order - 1)
            )
                ->simplified();

        return derivative;
    }

    auto const result = std::make_shared<Terms>();

    result->add_term(term);

    if (base_type == typeid(Constant)) {
        result->add_term(std::make_shared<Function>("ln", term->base));
        result->add_term(std::dynamic_pointer_cast<Differentiable>(term->power)
                             ->derivative(variable, 1));

        auto derivative = result->simplified();

        if (order > 1)
            return std::dynamic_pointer_cast<Simplifiable>(
                       std::dynamic_pointer_cast<Differentiable>(derivative)
                           ->derivative(variable, order - 1)
            )
                ->simplified();

        return derivative;
    }

    auto const terms_1 = std::make_shared<Terms>();
    terms_1->add_term(term->power);
    terms_1->add_term(std::dynamic_pointer_cast<Differentiable>(term->base)
                          ->derivative(variable, 1));
    terms_1->add_term(
        std::make_shared<Term>(term->base, std::make_shared<Constant>(-1))
    );

    auto const terms_2 = std::make_shared<Terms>();
    terms_2->add_term(std::dynamic_pointer_cast<Differentiable>(term->power)
                          ->derivative(variable, 1));
    terms_2->add_term(std::make_shared<Function>("ln", term->base));

    auto const expression = std::make_shared<Expression>();
    expression->add_token(terms_1);
    expression->add_token(std::make_shared<Operation>(Operation::add));
    expression->add_token(terms_2);

    result->add_term(expression);

    auto derivative = result->simplified();

    if (order > 1)
        return std::dynamic_pointer_cast<Simplifiable>(
                   std::dynamic_pointer_cast<Differentiable>(derivative)
                       ->derivative(variable, order - 1)
        )
            ->simplified();

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

            if (auto derivative =
                    std::dynamic_pointer_cast<Differentiable>(terms->terms[i])
                        ->derivative(variable, 1);
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
        return std::dynamic_pointer_cast<Simplifiable>(
                   std::dynamic_pointer_cast<Differentiable>(derivative)
                       ->derivative(variable, order - 1)
        )
            ->simplified();

    return derivative;
}

std::shared_ptr<Token> Expression::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return std::make_shared<Expression>(*this);

    auto const result = std::make_shared<Expression>();

    for (std::shared_ptr<Token> const &term : this->tokens) {
        if (auto const &token_type = typeid(*term);
            token_type == typeid(Operation)) {
            result->add_token(term);

            continue;
        }

        result->add_token(std::dynamic_pointer_cast<Differentiable>(term)
                              ->derivative(variable, 1));
    }

    auto derivative = result->simplified();

    if (order > 1)
        return std::dynamic_pointer_cast<Simplifiable>(
                   std::dynamic_pointer_cast<Differentiable>(derivative)
                       ->derivative(variable, order - 1)
        )
            ->simplified();

    return derivative;
}