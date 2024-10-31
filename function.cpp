#include "token.h"

#include <sstream>

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