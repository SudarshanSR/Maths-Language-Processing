#include "token.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <sstream>

namespace {
std::map<char, char> const k_parenthesis_map{
    {'(', ')'}, {'[', ']'}, {'{', '}'}
};

std::map<std::string, double (*)(double)> k_functions{
    {"sin", std::sin},
    {"cos", std::cos},
    {"tan", std::tan},
    {"sec", [](double const val) -> double { return 1 / std::cos(val); }},
    {"csc", [](double const val) -> double { return 1 / std::sin(val); }},
    {"cot", [](double const val) -> double { return 1 / std::tan(val); }},
    {"sinh", std::sinh},
    {"cosh", std::cosh},
    {"tanh", std::tanh},
    {"sech", [](double const val) -> double { return 1 / std::cosh(val); }},
    {"csch", [](double const val) -> double { return 1 / std::sinh(val); }},
    {"coth", [](double const val) -> double { return 1 / std::tanh(val); }},
    {"asin", std::asin},
    {"acos", std::acos},
    {"atan", std::atan},
    {"asec", [](double const val) -> double { return std::acos(1 / val); }},
    {"acsc", [](double const val) -> double { return std::asin(1 / val); }},
    {"acot", [](double const val) -> double { return std::atan(1 / val); }},
    {"asinh", std::asinh},
    {"acosh", std::acosh},
    {"atanh", std::atanh},
    {"asech", [](double const val) -> double { return std::acosh(1 / val); }},
    {"acsch", [](double const val) -> double { return std::asinh(1 / val); }},
    {"acoth", [](double const val) -> double { return std::atanh(1 / val); }},
    {"ln", std::log},
};

std::shared_ptr<Token> get_next_token(std::string const &expression, int &i) {
    if (i >= expression.size())
        return nullptr;

    char character = expression[i];

    if (auto op = Operation::from_char(character))
        return op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' <= character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i < expression.size()) {
            if (character != '.') {
                --i;

                return std::make_shared<Constant>(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return std::make_shared<Constant>(std::stold(number));
        }

        return std::make_shared<Constant>(std::stoull(number));
    }

    if (k_parenthesis_map.contains(character)) {
        int count = 1;
        char const original = character;

        character = expression[++i];

        std::size_t const start = i;

        while (i < expression.size() && count) {
            if (character == original)
                ++count;

            else if (character == k_parenthesis_map.at(original))
                --count;

            character = expression[++i];
        }

        --i;

        if (count)
            throw std::invalid_argument("Expression is not valid!");

        std::size_t const end = i - start;

        return tokenise(expression.substr(start, end));
    }

    for (int const offset : {5, 4, 3, 2}) {
        if (i + offset >= expression.size())
            continue;

        std::string const &fn = expression.substr(i, offset);

        if (!k_functions.contains(fn))
            continue;

        i += offset;

        return std::make_shared<Function>(fn, get_next_token(expression, i));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_shared<Variable>(character);

    return nullptr;
}
} // namespace

Constant::Constant(double const value) : value(value) {}

std::shared_ptr<Token>
Constant::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    return std::make_shared<Constant>(*this);
}

Constant::operator std::string() const { return std::to_string(this->value); }

bool Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

Variable::Variable(char const var) : var(var) {}

bool Variable::is_dependent_on(Variable const &variable) const {
    return *this == variable;
}

std::shared_ptr<Token>
Variable::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    return values.contains(*this) ? values.at(*this)
                                  : std::make_shared<Variable>(*this);
}

Variable::operator std::string() const { return {this->var}; }

bool Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

Operation::Operation(op const operation) : operation(operation) {}

std::shared_ptr<Operation> Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return std::make_shared<Operation>(add);

    case '-':
        return std::make_shared<Operation>(sub);

    case '*':
        return std::make_shared<Operation>(mul);

    case '/':
        return std::make_shared<Operation>(div);

    case '^':
        return std::make_shared<Operation>(pow);

    default:
        return nullptr;
    }
}

Operation::operator std::string() const {
    switch (this->operation) {
    case add:
        return "+";
    case sub:
        return "-";
    case mul:
        return "*";
    case div:
        return "/";
    case pow:
        return "^";
    }

    return "";
}

Function::Function(
    std::string function, std::shared_ptr<Token> const &parameter
)
    : function(std::move(function)), parameter(parameter) {}

bool Function::is_dependent_on(Variable const &variable) const {
    if (typeid(*this->parameter) == typeid(Constant))
        return false;

    return std::dynamic_pointer_cast<Dependent>(this->parameter)
        ->is_dependent_on(variable);
}

std::shared_ptr<Token>
Function::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    auto param =
        std::dynamic_pointer_cast<Evaluatable>(this->parameter)->at(values);

    if (typeid(*param) == typeid(Constant))
        return std::make_shared<Constant>(k_functions.at(this->function)(
            std::dynamic_pointer_cast<Constant>(param)->value
        ));

    return std::make_shared<Function>(this->function, param);
}

Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

Term::Term(
    double const coefficient, std::shared_ptr<Token> const &base,
    std::shared_ptr<Token> const &power
)
    : coefficient(coefficient), base(base), power(power) {}

Term::Term(
    std::shared_ptr<Token> const &base, std::shared_ptr<Token> const &power
)
    : base(base), power(power) {}

bool Term::is_dependent_on(Variable const &variable) const {
    if (typeid(*this->base) == typeid(Constant) &&
        typeid(*this->power) == typeid(Constant))
        return false;

    return std::dynamic_pointer_cast<Dependent>(this->base)
               ->is_dependent_on(variable) ||
           std::dynamic_pointer_cast<Dependent>(this->power)
               ->is_dependent_on(variable);
}

std::shared_ptr<Token>
Term::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    auto const term = std::make_shared<Term>(*this);
    term->base = std::dynamic_pointer_cast<Evaluatable>(term->base)->at(values);
    term->power =
        std::dynamic_pointer_cast<Evaluatable>(term->power)->at(values);

    return term->simplified();
}

Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    result << *this->base;

    if (this->power) {
        if (typeid(*this->power) == typeid(Constant)) {
            if (std::dynamic_pointer_cast<Constant>(this->power)->value != 1) {
                result << '^';
                result << *this->power;
            }
        } else {
            result << '^';
            result << *this->power;
        }
    }

    result << ')';

    return result.str();
}

void Terms::add_term(std::shared_ptr<Token> const &token) {
    auto const &term_type = typeid(*token);

    if (term_type == typeid(Constant)) {
        this->coefficient.value *=
            std::dynamic_pointer_cast<Constant>(token)->value;

        return;
    }

    if (term_type == typeid(Term)) {
        auto const term = std::dynamic_pointer_cast<Term>(token);
        this->coefficient.value *= term->coefficient.value;
        term->coefficient.value = 1;

        this->terms.push_back(term);

        return;
    }

    if (term_type == typeid(Terms)) {
        auto const terms = std::dynamic_pointer_cast<Terms>(token);
        this->coefficient.value *= terms->coefficient.value;
        terms->coefficient.value = 1;

        for (std::shared_ptr<Token> const &term : terms->terms)
            this->add_term(term);

        return;
    }

    this->terms.push_back(token);
}

bool Terms::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->terms,
        [variable](std::shared_ptr<Token> const &token) -> bool {
            return typeid(*token) != typeid(Constant) &&
                   std::dynamic_pointer_cast<Dependent>(token)->is_dependent_on(
                       variable
                   );
        }
    );
}

std::shared_ptr<Token>
Terms::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    auto const terms = std::make_shared<Terms>(*this);

    for (auto &term : terms->terms)
        term = std::dynamic_pointer_cast<Evaluatable>(term)->at(values);

    return terms->simplified();
}

Terms::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    for (int i = 0; i < this->terms.size(); ++i) {
        result << static_cast<std::string>(*this->terms[i]);

        if (i != this->terms.size() - 1)
            result << '*';
    }

    result << ')';

    return result.str();
}

void Expression::add_token(std::shared_ptr<Token> const &token) {
    this->tokens.push_back(token);
}

std::shared_ptr<Token> Expression::pop_token() {
    std::shared_ptr<Token> token = this->tokens.back();

    this->tokens.pop_back();

    return token;
}

bool Expression::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->tokens,
        [variable](std::shared_ptr<Token> const &token) -> bool {
            return typeid(*token) != typeid(Constant) &&
                   typeid(*token) != typeid(Operation) &&
                   std::dynamic_pointer_cast<Dependent>(token)->is_dependent_on(
                       variable
                   );
        }
    );
}

std::shared_ptr<Token>
Expression::at(std::map<Variable, std::shared_ptr<Token>> const &values) {
    auto const expression = std::make_shared<Expression>(*this);

    for (auto &term : expression->tokens)
        if (typeid(*term) != typeid(Operation))
            term = std::dynamic_pointer_cast<Evaluatable>(term)->at(values);

    return expression->simplified();
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << static_cast<std::string>(*this->tokens[0]);
    } else {
        result << '(';

        for (std::shared_ptr<Token> const &token : this->tokens)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

std::shared_ptr<Token> tokenise(std::string expression) {
    auto const result = std::make_shared<Expression>();

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    auto terms = std::make_shared<Terms>();

    for (int i = 0; i < expression.size(); ++i) {
        std::int32_t const copy = i;

        std::shared_ptr<Token> token = get_next_token(expression, i);

        if (!token)
            continue;

        if (typeid(*token) == typeid(Operation)) {
            auto operation = std::dynamic_pointer_cast<Operation>(token);

            if (operation->operation == Operation::add ||
                operation->operation == Operation::sub) {
                if (!terms->terms.empty())
                    result->add_token(terms);

                else if (copy != 0)
                    result->add_token(
                        std::make_shared<Constant>(terms->coefficient)
                    );

                terms = std::make_shared<Terms>();
                result->add_token(operation);

                continue;
            }

            if (operation->operation == Operation::mul) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value *=
                        std::dynamic_pointer_cast<Constant>(next)->value;
                } else {
                    terms->add_term(next);
                }

                continue;
            }

            if (operation->operation == Operation::div) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value /=
                        std::dynamic_pointer_cast<Constant>(next)->value;
                } else {
                    terms->add_term(std::make_shared<Term>(
                        1, next, std::make_shared<Constant>(-1)
                    ));
                }

                continue;
            }

            std::vector<std::shared_ptr<Term>> powers;

            std::shared_ptr<Token> next;

            while (true) {
                next = get_next_token(expression, ++i);

                if (!next)
                    break;

                if (typeid(*next) == typeid(Operation)) {
                    operation = std::dynamic_pointer_cast<Operation>(next);

                    if (operation->operation == Operation::pow)
                        continue;

                    break;
                }

                if (typeid(*next) == typeid(Term)) {
                    powers.push_back(std::dynamic_pointer_cast<Term>(next));
                } else {
                    powers.push_back(std::make_shared<Term>(
                        1, next, std::make_shared<Constant>(1)
                    ));
                }
            }

            std::shared_ptr<Token> power = std::make_shared<Constant>(1);

            for (std::shared_ptr<Term> const &p :
                 powers | std::views::reverse) {
                power = std::make_shared<Term>(1, p, power);
            }

            terms->terms.back() =
                std::make_shared<Term>(1, terms->terms.back(), power);

            if (next) {
                --i;
            }

            continue;
        }

        if (typeid(*token) == typeid(Constant)) {
            terms->terms.push_back(token);
        } else {
            terms->add_term(token);
        }
    }

    if (!terms->terms.empty())
        result->add_token(terms);

    else
        result->add_token(std::make_shared<Constant>(terms->coefficient));

    return result->simplified();
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}