#include "token.h"

#include <iostream>
#include <numbers>

int main(int argc, char *argv[]) {
    auto const expression = tokenise("sin(x)");

    std::cout << *expression << '\n';

    auto const derivative = dynamic_cast<Differentiable &>(*expression)
                                .derivative(Variable('x'), 1);
    std::cout << "Derivative: " << *derivative << '\n';

    std::cout << "At x = 1: "
              << *dynamic_cast<Differentiable &>(*expression)
                      .derivative(
                          Variable('x'), 1,
                          {{Variable('x'), std::make_shared<Constant>(1)}}
                      )
              << '\n';

    auto const integral =
        dynamic_cast<Integrable &>(*expression).integral(Variable('x'));
    std::cout << "Integral: " << *integral << '\n';

    std::cout << "From x = 0 to pi: "
              << *dynamic_cast<Integrable &>(*expression)
                      .integral(
                          Variable('x'), std::make_shared<Constant>(0),
                          std::make_shared<Constant>(std::numbers::pi)
                      )
              << '\n';
}