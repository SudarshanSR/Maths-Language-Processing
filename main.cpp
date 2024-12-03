#include "include/token.h"

#include "include/expression.h"
#include "include/function.h"
#include "include/term.h"
#include "include/terms.h"
#include "include/variable.h"

#include <iostream>
#include <numbers>

using namespace mlp;

std::int32_t main(std::int32_t argc, char *argv[]) {
    Variable x{'x'}, y{'y'}, z{'z'};
    x *= 2;
    auto expression = x + x;

    std::cout << expression << '\n';

    std::cout << "Derivative: " << derivative(expression, x, 1) << '\n';

    std::cout << "At x = 1: " << derivative(expression, x, 1, {{x, 1.0}})
              << '\n';

    auto const integral = mlp::integral(expression, x);
    std::cout << "Integral: " << integral << '\n';

    std::cout << "From x = 0 to 1: " << mlp::integral(expression, x, 0.0, 1.0)
              << '\n';
}