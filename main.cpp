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

    std::cout << "Derivative: " << from_variant(derivative(expression, x, 1))
              << '\n';

    std::cout << "At x = 1: "
              << from_variant(derivative(
                     expression, x, 1, {{x, std::make_shared<Constant>(1)}}
                 ))
              << '\n';

    auto const integral = mlp::integral(expression, x);
    std::cout << "Integral: " << from_variant(integral) << '\n';

    std::cout << "From x = 0 to 1: "
              << from_variant(
                     mlp::integral(
                         expression, x, std::make_shared<Constant>(0),
                         std::make_shared<Constant>(1)
                     )
                 )
              << '\n';
}