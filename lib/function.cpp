#include "../include/function.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <map>
#include <sstream>

namespace {
std::map<std::string, std::double_t (*)(std::double_t)> k_functions{
    {"sin", std::sin},
    {"cos", std::cos},
    {"tan", std::tan},
    {"sec",
     [](std::double_t const val) -> std::double_t { return 1 / std::cos(val); }
    },
    {"csc",
     [](std::double_t const val) -> std::double_t { return 1 / std::sin(val); }
    },
    {"cot",
     [](std::double_t const val) -> std::double_t { return 1 / std::tan(val); }
    },
    {"sinh", std::sinh},
    {"cosh", std::cosh},
    {"tanh", std::tanh},
    {"sech",
     [](std::double_t const val) -> std::double_t { return 1 / std::cosh(val); }
    },
    {"csch",
     [](std::double_t const val) -> std::double_t { return 1 / std::sinh(val); }
    },
    {"coth",
     [](std::double_t const val) -> std::double_t { return 1 / std::tanh(val); }
    },
    {"asin", std::asin},
    {"acos", std::acos},
    {"atan", std::atan},
    {"asec",
     [](std::double_t const val) -> std::double_t { return std::acos(1 / val); }
    },
    {"acsc",
     [](std::double_t const val) -> std::double_t { return std::asin(1 / val); }
    },
    {"acot",
     [](std::double_t const val) -> std::double_t { return std::atan(1 / val); }
    },
    {"asinh", std::asinh},
    {"acosh", std::acosh},
    {"atanh", std::atanh},
    {"asech",
     [](std::double_t const val) -> std::double_t {
         return std::acosh(1 / val);
     }},
    {"acsch",
     [](std::double_t const val) -> std::double_t {
         return std::asinh(1 / val);
     }},
    {"acoth",
     [](std::double_t const val) -> std::double_t {
         return std::atanh(1 / val);
     }},
    {"ln", std::log},
    {"abs", std::abs}
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

mlp::Function::Function(std::string function, OwnedToken &&parameter)
    : function(std::move(function)), parameter(std::move(parameter)) {}

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

mlp::FunctionFactory::FunctionFactory(std::string function)
    : function(std::move(function)) {}

mlp::Function mlp::FunctionFactory::operator()(Token const &token) const {
    return Function(this->function, std::make_unique<Token>(token));
}

namespace mlp {
bool is_dependent_on(Function const &token, Variable const &variable) {
    return is_dependent_on(*token.parameter, variable);
}

bool is_linear_of(Function const &, Variable const &) { return false; }

Token evaluate(Function const &token, std::map<Variable, Token> const &values) {
    auto param = evaluate(*token.parameter, values);

    if (std::holds_alternative<Constant>(param))
        return Constant(k_functions.at(token.function)(std::get<Constant>(param)
        ));

    return Function(token.function, std::make_unique<Token>(std::move(param)));
}

Token simplified(Function const &token) {
    Token simplified = mlp::simplified(*token.parameter);

    if (std::holds_alternative<Constant>(simplified))
        return evaluate(
            Function(
                token.function, std::make_unique<Token>(std::move(simplified))
            ),
            {}
        );

    return Function(
        token.function, std::make_unique<Token>(std::move(simplified))
    );
}

Token derivative(
    Function const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

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

Token integral(Function const &token, Variable const &variable) {
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