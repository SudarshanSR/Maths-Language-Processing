#include "differentiate.h"
#include "tokenise.h"
#include <boost/core/demangle.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
    Expression const expression = tokenise("x^2 - (2x - {3 + 5})");

    std::cout << expression << '\n';

    std::cout << differentiate(expression, Variable('x')) << '\n';
}