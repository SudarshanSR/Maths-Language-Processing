#include "../include/function.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

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
};

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

mlp::Function::Function(std::string function, OwnedToken &&parameter)
    : function(std::move(function)), parameter(std::move(parameter)) {}

mlp::Function::Function(Function const &function)
    : function(function.function), parameter(function.parameter->clone()) {}

gsl::owner<mlp::Function *> mlp::Function::clone() const {
    return new Function(*this);
}

gsl::owner<mlp::Function *> mlp::Function::move() && {
    return new Function(std::move(*this));
}

mlp::Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

mlp::FunctionFactory::FunctionFactory(std::string function)
    : function(std::move(function)) {}

mlp::Function mlp::FunctionFactory::operator()(Token const &token) const {
    return Function(this->function, OwnedToken(token.clone()));
}

namespace mlp {
bool is_dependent_on(Function const &token, Variable const &variable) {
    return is_dependent_on(to_variant(*token.parameter), variable);
}

bool is_linear_of(Function const &, Variable const &) { return false; }

token evaluate(
    Function const &token, std::map<Variable, SharedToken> const &values
) {
    auto param = evaluate(to_variant(*token.parameter), values);

    if (std::holds_alternative<Constant>(param))
        return Constant(k_functions.at(token.function)(std::get<Constant>(param)
        ));

    return Function(
        token.function, OwnedToken(from_variant(std::move(param)).move())
    );
}

token simplified(Function const &token) {
    mlp::token simplified = mlp::simplified(*token.parameter);

    if (std::holds_alternative<Constant>(simplified))
        return evaluate(
            Function(
                token.function,
                OwnedToken(from_variant(std::move(simplified)).move())
            ),
            {}
        );

    return Function(
        token.function, OwnedToken(from_variant(std::move(simplified)).move())
    );
}

token derivative(
    Function const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

    auto parameter = static_cast<std::string>(*token.parameter);

    auto derivative = simplified(
        *tokenise(
            std::vformat(
                k_function_map.at(token.function),
                std::make_format_args(parameter)
            )
        ) *
        from_variant(mlp::derivative(to_variant(*token.parameter), variable, 1))
    );

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

token integral(Function const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    if (is_linear_of(to_variant(*token.parameter), variable)) {
        auto string = static_cast<std::string>(*token.parameter);

        return simplified(
            *tokenise(
                std::vformat(
                    k_function_map.at(token.function),
                    std::make_format_args(string)
                )
            ) *
            from_variant(derivative(to_variant(*token.parameter), variable, 1))
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