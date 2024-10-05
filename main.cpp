#include "differentiate.h"
#include "token.h"

#include <iostream>

int main(int argc, char *argv[]) {
    Expression const expression = tokenise("cos(x)");

    std::cout << expression << '\n';

    auto const *derivative = differentiate(expression, Variable('x'));

    std::cout << *derivative << '\n';

    delete derivative;
}