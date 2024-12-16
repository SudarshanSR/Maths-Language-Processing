#include "include/expression.h"
#include "include/function.h"
#include "include/term.h"
#include "include/terms.h"
#include "include/token.h"
#include "include/variable.h"

#include <iostream>
#include <ranges>
#include <sstream>

using namespace mlp;

std::int32_t get_choice(std::vector<std::string> const &choices) {
    std::stringstream stream;

    for (std::int32_t i = 0; i < choices.size(); ++i)
        stream << i + 1 << ')' << ' ' << choices[i] << '\n';

    stream << "Enter choice: ";

    std::string const display = stream.str();

    std::int32_t op;

    while (true) {
        std::cout << display;
        std::cin >> op;

        if (op >= 1 && op <= choices.size())
            break;

        std::cerr << "Invalid choice! Please select again!\n";
    }

    return op;
}

std::map<Variable, Token> get_values() {
    std::cout << "Input values to evaluate at in the format of "
                 "{variable}={value} separated by spaces:\n";

    std::cin.ignore(); // Figure out why std::cin has a \n inserted

    std::string line;
    std::getline(std::cin, line);

    std::map<Variable, Token> values;

    for (auto sub : line | std::views::split(' ') |
                        std::ranges::to<std::vector<std::string>>())
        values[Variable{sub[0]}] = tokenise(sub.substr(2));

    return values;
}

std::int32_t main(std::int32_t argc, char *argv[]) {
    std::cout << "Enter expression: ";

    Token input;

    std::cin >> input;

    std::int32_t choice = get_choice(
        {"Simplify", "Differentiate", "Integrate (incomplete)", "Evaluate"}
    );

    if (choice == 1) {
        std::cout << input << " simplified is " << simplified(input) << '\n';
    } else if (choice == 2) {
        std::cout << "Enter variable to differentiate with respect to: ";
        Variable var;
        std::cin >> var;

        std::cout << "Enter order of differentiation: ";
        std::uint32_t order;
        std::cin >> order;

        choice = get_choice({"Evaluate at value", "General derivative"});

        if (choice == 1) {
            std::map<Variable, Token> const values = get_values();

            std::cout << order << " derivative of " << input
                      << " with respect to " << var
                      << " evaluated under given conditions is is "
                      << derivative(input, var, order, values) << '\n';
        } else {
            std::cout << order << " derivative of " << input
                      << " with respect to " << var << " is "
                      << derivative(input, var, order) << '\n';
        }
    } else if (choice == 3) {
        std::cout << "Enter variable to integrate with respect to: ";
        Variable var;
        std::cin >> var;

        choice = get_choice({"Definite integral", "Indefinite integral"});

        if (choice == 1) {
            std::cout << "Enter lower and upper limits of integration "
                         "separated by space: ";
            Token lower, upper;
            std::cin >> lower >> upper;

            std::cout << "Integral of " << input << " with respect to " << var
                      << " from " << lower << " to " << upper << " is "
                      << integral(input, var, lower, upper) << '\n';
        } else {
            std::cout << "Integral of " << input << " with respect to " << var
                      << " is " << integral(input, var) << '\n';
        }
    } else if (choice == 4) {
        std::map<Variable, Token> const values = get_values();

        std::cout << input << " evaluated under given conditions is "
                  << evaluate(input, values) << '\n';
    }

    return 0;
}