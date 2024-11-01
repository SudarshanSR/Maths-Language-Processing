#include "include/token.h"

#include "include/constant.h"
#include "include/variable.h"

#include <iostream>
#include <numbers>

std::int32_t main(std::int32_t argc, char *argv[]) {
    auto const expression = mlp::tokenise("x^2 - x - x/2");

    std::cout << *expression << '\n';

    auto const derivative = expression->derivative(mlp::Variable('x'), 1);
    std::cout << "Derivative: " << *derivative << '\n';

    std::cout << "At x = 1: "
              << *expression->derivative(
                     mlp::Variable('x'), 1,
                     {{mlp::Variable('x'), std::make_shared<mlp::Constant>(1)}}
                 )
              << '\n';

    auto const integral = expression->integral(mlp::Variable('x'));
    std::cout << "Integral: " << *integral << '\n';

    std::cout << "From x = 0 to 1: "
              << *expression->integral(
                     mlp::Variable('x'), std::make_shared<mlp::Constant>(0),
                     std::make_shared<mlp::Constant>(1)
                 )
              << '\n';
}