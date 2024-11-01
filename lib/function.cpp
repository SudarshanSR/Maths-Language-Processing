#include "../include/function.h"

#include "../include/constant.h"
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
    return new Function(this->function, OwnedToken(this->parameter->clone()));
}

mlp::Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

bool mlp::Function::is_dependent_on(Variable const &variable) const {
    return this->parameter->is_dependent_on(variable);
}

bool mlp::Function::is_linear_of(Variable const &variable) const {
    return false;
}

mlp::OwnedToken
mlp::Function::evaluate(std::map<Variable, SharedToken> const &values) const {
    auto param = this->parameter->evaluate(values);

    if (typeid(*param) == typeid(Constant))
        return std::make_unique<Constant>(k_functions.at(this->function)(
            dynamic_cast<Constant const &>(*param)
        ));

    return std::make_unique<Function>(this->function, std::move(param));
}

mlp::OwnedToken mlp::Function::simplified() const {
    OwnedToken simplified = this->parameter->simplified();

    if (typeid(*simplified) == typeid(Constant))
        return Function(this->function, std::move(simplified)).evaluate({});

    return std::make_unique<Function>(this->function, std::move(simplified));
}

mlp::OwnedToken mlp::Function::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    auto parameter = static_cast<std::string>(*this->parameter);

    Terms result{};
    result *= tokenise(
        std::vformat(
            k_function_map.at(this->function), std::make_format_args(parameter)
        )
    );
    result *= this->parameter->derivative(variable, 1);

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Function::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Owned<Variable>(variable.clone());
        *terms *= Owned<Function>(this->clone());

        return terms;
    }

    if (this->parameter->is_linear_of(variable)) {
        Terms terms{};

        auto string = static_cast<std::string>(*this->parameter);

        terms *= tokenise(
            std::vformat(
                k_function_map.at(this->function), std::make_format_args(string)
            )
        );
        terms /= this->parameter->derivative(variable, 1)->simplified();

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}