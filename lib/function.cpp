#include "../include/function.h"

#include "../include/expression.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <map>
#include <sstream>

namespace {
std::map<std::string, mlp::Constant (*)(mlp::Constant)> k_functions{
    {"sin", std::sin},
    {"cos", std::cos},
    {"tan", std::tan},
    {"sec",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::cos(val); }
    },
    {"csc",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::sin(val); }
    },
    {"cot",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::tan(val); }
    },
    {"sinh", std::sinh},
    {"cosh", std::cosh},
    {"tanh", std::tanh},
    {"sech",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::cosh(val); }
    },
    {"csch",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::sinh(val); }
    },
    {"coth",
     [](mlp::Constant const val) -> mlp::Constant { return 1 / std::tanh(val); }
    },
    {"asin", std::asin},
    {"acos", std::acos},
    {"atan", std::atan},
    {"asec",
     [](mlp::Constant const val) -> mlp::Constant { return std::acos(1 / val); }
    },
    {"acsc",
     [](mlp::Constant const val) -> mlp::Constant { return std::asin(1 / val); }
    },
    {"acot",
     [](mlp::Constant const val) -> mlp::Constant { return std::atan(1 / val); }
    },
    {"asinh", std::asinh},
    {"acosh", std::acosh},
    {"atanh", std::atanh},
    {"asech",
     [](mlp::Constant const val) -> mlp::Constant {
         return std::acosh(1 / val);
     }},
    {"acsch",
     [](mlp::Constant const val) -> mlp::Constant {
         return std::asinh(1 / val);
     }},
    {"acoth",
     [](mlp::Constant const val) -> mlp::Constant {
         return std::atanh(1 / val);
     }},
    {"ln", std::log},
    {"abs", std::abs}
};

std::map<std::string, std::string> k_inverses{
    {"sin", "asin"},   {"cos", "acos"},   {"tan", "atan"},   {"sec", "asec"},
    {"csc", "acsc"},   {"cot", "acot"},   {"sinh", "asinh"}, {"cosh", "acosh"},
    {"tanh", "atanh"}, {"sech", "asech"}, {"csch", "acsch"}, {"coth", "acoth"},
    {"asin", "sin"},   {"acos", "cos"},   {"atan", "tan"},   {"asec", "sec"},
    {"acsc", "csc"},   {"acot", "cot"},   {"asinh", "sinh"}, {"acosh", "cosh"},
    {"atanh", "tanh"}, {"asech", "sech"}, {"acsch", "csch"}, {"acoth", "coth"}
};

std::map<std::string, std::string> k_derivative_map{
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
    {"asec", "1/(abs({0})*(({0})^2 - 1)^0.5"},
    {"acsc", "-1/(abs({0})*(({0})^2 - 1)^0.5"},
    {"acot", "-1/(1 + ({0})^2)"},
    {"asinh", "1/((1 + ({0})^2)^0.5)"},
    {"acosh", "-1/((1 + ({0})^2)^0.5)"},
    {"atanh", "1/(1 - ({0})^2)"},
    {"asech", "-1/(({0})*(1 - ({0})^2)^0.5"},
    {"acsch", "1/(abs({0})*(1 - ({0})^2)^0.5"},
    {"acoth", "1/(1 - ({0})^2)"},
    {"ln", "1/({0})"},
    {"abs", "abs({0})/({0})"}
};

std::map<std::string, std::string> k_integral_map{
    {"sin", "-cos({0})"},
    {"cos", "sin({0})"},
    {"tan", "ln(abs(sec({0})))"},
    {"sec", "ln(abs(sec({0}) + tan({0})))"},
    {"csc", "ln(abs(sin({0})))"},
    {"cot", "ln(abs(csc({0}) - cot({0})))"},
    {"sinh", "cosh({0})"},
    {"cosh", "sinh({0})"},
    {"tanh", "ln(cosh({0}))"},
    {"sech", "atan(sinh({0}))"},
    {"csch", "ln(abs(coth({0}) - csch({0})))"},
    {"coth", "ln(abs(sinh({0})))"},
    {"asin", "({0})asin({0}) + ((1 - ({0})^2)^0.5"},
    {"acos", "({0})acos({0}) - ((1 - ({0})^2)^0.5"},
    {"atan", "({0})atan({0}) - ln(abs(1 + ({0})^2))/2"},
    {"asec", "({0})asec({0}) - acosh(abs({0}))"},
    {"acsc", "({0})acsc({0}) + acosh(abs({0}))"},
    {"acot", "({0})acot({0}) + ln(abs(1 + ({0})^2))/2"},
    {"asinh", "({0})asinh({0}) - ((1 + ({0})^2)^0.5"},
    {"acosh", "({0})acosh({0}) - ((1 - ({0})^2)^0.5"},
    {"atanh", "({0})atanh({0}) + ln(1 - ({0})^2)/2"},
    {"asech", "({0})asech({0}) - 2atan(((1 - ({0})/(1 - ({0}))))^0.5)"},
    {"acsch", "({0})acsch({0}) + acoth((1 + ({0})^2)^0.5)/({0})"},
    {"acoth", "({0})acoth({0}) + ln(({0})^2 - 1)/2"},
    {"ln", "({0})ln(abs({0})) - ({0})"},
    {"abs", "({0})abs({0})/2"}
};
} // namespace

mlp::Function::Function(std::string function, Token parameter)
    : function(std::move(function)),
      parameter(new Token(std::move(parameter))) {}

mlp::Function::Function(Function const &function)
    : function(function.function), parameter(new Token(*function.parameter)) {}

mlp::Function &mlp::Function::operator=(Function const &function) {
    this->function = function.function;
    *this->parameter = *function.parameter;

    return *this;
}

mlp::Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '(' << to_string(*this->parameter) << ')';

    return result.str();
}

bool mlp::Function::operator==(Function const &rhs) const {
    return this->function == rhs.function && *this->parameter == *rhs.parameter;
}

mlp::FunctionFactory::FunctionFactory(std::string function)
    : function(std::move(function)) {}

mlp::Function mlp::FunctionFactory::operator()(Token const &token) const {
    return {this->function, token};
}

mlp::Term mlp::operator-(Function token) { return {-1, token, 1.0}; }

mlp::Token mlp::operator+(Function lhs, Constant const rhs) {
    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Function lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Function const &lhs, Function const &rhs) {
    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Function lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Function lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Function const &lhs, Expression rhs) {
    return rhs += lhs;
}

mlp::Token mlp::operator-(Function lhs, Constant const rhs) {
    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Function lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Function const &lhs, Function const &rhs) {
    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Function lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Function lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Function const &lhs, Expression rhs) {
    return -(rhs -= lhs);
}

mlp::Token mlp::operator*(Function lhs, Constant const rhs) {
    if (rhs == 0)
        return 0.0;

    if (rhs == 1)
        return lhs;

    return Term{rhs, lhs, 1.0};
}

mlp::Token mlp::operator*(Function const &lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Function const &lhs, Function const &rhs) {
    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Function const &lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Function const &lhs, Terms rhs) {
    if (rhs.coefficient == 0)
        return 0.0;

    return rhs *= lhs;
}

mlp::Token mlp::operator*(Function const &lhs, Expression rhs) {
    return rhs *= lhs;
}

mlp::Token mlp::operator/(Function lhs, Constant const rhs) {
    if (rhs == 0)
        throw std::domain_error{"Division by 0!"};

    if (rhs == 1)
        return lhs;

    return Term{1.0 / rhs, lhs, 1.0};
}

mlp::Token mlp::operator/(Function const &lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Function const &lhs, Function const &rhs) {
    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Function const &lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Function const &lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    return rhs * pow(lhs, -1);
}

mlp::Token mlp::operator/(Function const &lhs, Expression const &rhs) {
    return pow(rhs / lhs, -1);
}

mlp::Token mlp::pow(Function const &lhs, Constant const rhs) {
    if (rhs == 0)
        return 1.0;

    if (rhs == 1)
        return lhs;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Function const &lhs, Variable rhs) {
    if (rhs.coefficient == 0)
        return 1.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Function const &lhs, Function const &rhs) {
    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Function const &lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        return 1.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Function const &lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        return 1.0;

    return Term{1, lhs, rhs};
}

namespace mlp {
bool is_dependent_on(Function const &token, Variable const variable) {
    return is_dependent_on(*token.parameter, variable);
}

bool is_linear_of(Function const &, Variable) { return false; }

Token evaluate(Function const &token, std::map<Variable, Token> const &values) {
    auto param = evaluate(*token.parameter, values);

    if (std::holds_alternative<Constant>(param))
        return k_functions.at(token.function)(std::get<Constant>(param));

    return Function(token.function, std::move(param));
}

Token simplified(Function const &token) {
    Token simplified = mlp::simplified(*token.parameter);

    if (std::holds_alternative<Function>(simplified))
        if (auto const &parameter = std::get<Function>(simplified);
            k_inverses.contains(token.function) &&
            parameter.function == k_inverses[token.function])
            return mlp::simplified(*parameter.parameter);

    if (std::holds_alternative<Constant>(simplified))
        return evaluate(Function(token.function, std::move(simplified)), {});

    if (token.function == "ln") {
        if (std::holds_alternative<Variable>(simplified)) {
            auto &variable = std::get<Variable>(simplified);

            Constant const coefficient = variable.coefficient;

            variable.coefficient = 1;

            return k_functions["ln"](coefficient) + "ln"_f(variable);
        }

        if (std::holds_alternative<Term>(simplified))
            if (auto const &term = std::get<Term>(simplified);
                !std::holds_alternative<Constant>(*term.power) ||
                std::get<Constant>(*term.power) != 1)
                return mlp::simplified(
                    *term.power * Function("ln", *term.base)
                );

        if (std::holds_alternative<Terms>(simplified)) {
            auto &terms = std::get<Terms>(simplified);

            Expression result;

            result += k_functions["ln"](terms.coefficient);

            for (Token const &t : terms.terms)
                result += mlp::simplified("ln"_f(t));

            return mlp::simplified(result);
        }
    }

    return Function(token.function, std::move(simplified));
}

Token derivative(
    Function const &token, Variable const variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return 0.0;

    auto parameter = to_string(*token.parameter);

    auto derivative = simplified(
        tokenise(
            std::vformat(
                k_derivative_map.at(token.function),
                std::make_format_args(parameter)
            )
        ) *
        mlp::derivative(*token.parameter, variable, 1)
    );

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

Token integral(Function const &token, Variable const variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    if (is_linear_of(*token.parameter, variable)) {
        auto string = to_string(*token.parameter);

        return simplified(
            tokenise(
                std::vformat(
                    k_integral_map.at(token.function),
                    std::make_format_args(string)
                )
            ) *
            derivative(*token.parameter, variable, 1)
        );
    }

    throw std::runtime_error("Expression is not integrable!");
}
} // namespace mlp

mlp::FunctionFactory operator""_f(char const *string, size_t) {
    if (!k_functions.contains(string))
        throw std::runtime_error(std::format("Unknown function {}!", string));

    return mlp::FunctionFactory(string);
}