#include "token.h"

mlp::Operation::Operation(op const operation) : operation(operation) {}

std::optional<mlp::Operation> mlp::Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return Operation(add);

    case '-':
        return Operation(sub);

    case '*':
        return Operation(mul);

    case '/':
        return Operation(div);

    case '^':
        return Operation(pow);

    default:
        return {};
    }
}

mlp::Operation::operator std::string() const {
    switch (this->operation) {
    case add:
        return "+";
    case sub:
        return "-";
    case mul:
        return "*";
    case div:
        return "/";
    case pow:
        return "^";
    }

    return "";
}