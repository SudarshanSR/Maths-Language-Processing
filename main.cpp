#include "differentiate.h"
#include "token.h"

#include <iostream>

int main(int argc, char *argv[]) {
    auto const expression = tokenise("x ^ y");

    std::cout << *expression << '\n';

    auto const first = differentiate(expression, Variable('x'), 1);

    std::cout << *first << '\n';

    std::cout << *differentiate(first, Variable('x'), 1) << '\n';
    std::cout << *differentiate(expression, Variable('x'), 2) << '\n';
}