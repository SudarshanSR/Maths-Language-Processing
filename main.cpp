#include "include/token.h"

#include "include/constant.h"
#include "include/expression.h"
#include "include/function.h"
#include "include/term.h"
#include "include/terms.h"
#include "include/variable.h"

#include <iostream>
#include <numbers>

using namespace mlp;

std::int32_t main(std::int32_t argc, char *argv[]) {
    Variable const x{'x'}, y{'y'}, z{'z'};
    auto const expression = x + y + z;

    std::cout << expression << '\n';

    std::cout << "Derivative: " << derivative(expression, x, 1) << '\n';

    std::cout << "At x = 1: "
              << derivative(expression, x, 1, {{x, Constant(1)}}) << '\n';

    auto const integral = mlp::integral(expression, x);
    std::cout << "Integral: " << integral << '\n';

    std::cout << "From x = 0 to 1: "
              << mlp::integral(expression, x, Constant(0), Constant(1)) << '\n';
}