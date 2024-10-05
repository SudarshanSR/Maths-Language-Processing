#include "differentiate.h"
#include "tokenise.h"

#include <iostream>

int main(int argc, char *argv[]) {
    Expression const expression = tokenise("x + y");

    std::cout << expression << '\n';

    auto const *derivative = differentiate(expression, Variable('x'));

    std::cout << *derivative << '\n';
}