#include "differentiate.h"
#include "token.h"

#include <iostream>

int main(int argc, char *argv[]) {
    auto const expression = tokenise("x^x^x");

    std::cout << *expression << '\n';

    auto const first = differentiate(expression, Variable('x'), 1);

    std::cout << *first << '\n';
}