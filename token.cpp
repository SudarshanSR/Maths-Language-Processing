#include "token.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <sstream>
#include <utility>

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

OwnedToken get_next_token(std::string const &expression, int &i) {
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

                return std::make_unique<Constant>(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return std::make_unique<Constant>(std::stold(number));
        }

        return std::make_unique<Constant>(std::stoull(number));
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

        return std::make_unique<Function>(fn, get_next_token(expression, i));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_unique<Variable>(character);

    return nullptr;
}
} // namespace

Constant::Constant(double const value) : value(value) {}

OwnedToken Constant::clone() const {
    return std::make_unique<Constant>(this->value);
}

OwnedToken Constant::at(std::map<Variable, SharedToken> const &values) const {
    return this->clone();
}

Constant::operator std::string() const { return std::to_string(this->value); }

bool Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

Variable::Variable(char const var) : var(var) {}

OwnedToken Variable::clone() const {
    return std::make_unique<Variable>(this->var);
}

bool Variable::is_dependent_on(Variable const &variable) const {
    return *this == variable;
}

OwnedToken Variable::at(std::map<Variable, SharedToken> const &values) const {
    return values.contains(*this) ? values.at(*this)->clone() : this->clone();
}

Variable::operator std::string() const { return {this->var}; }

bool Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

Operation::Operation(op const operation) : operation(operation) {}

OwnedToken Operation::clone() const {
    return std::make_unique<Operation>(this->operation);
}

std::unique_ptr<Operation> Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return std::make_unique<Operation>(add);

    case '-':
        return std::make_unique<Operation>(sub);

    case '*':
        return std::make_unique<Operation>(mul);

    case '/':
        return std::make_unique<Operation>(div);

    case '^':
        return std::make_unique<Operation>(pow);

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

Function::Function(std::string function, OwnedToken &&parameter)
    : function(std::move(function)), parameter(std::move(parameter)) {}

OwnedToken Function::clone() const {
    return std::make_unique<Function>(this->function, this->parameter->clone());
}

bool Function::is_dependent_on(Variable const &variable) const {
    if (typeid(*this->parameter) == typeid(Constant))
        return false;

    return dynamic_cast<Dependent &>(*this->parameter)
        .is_dependent_on(variable);
}

OwnedToken Function::at(std::map<Variable, SharedToken> const &values) const {
    auto param = dynamic_cast<Evaluatable &>(*this->parameter).at(values);

    if (typeid(*param) == typeid(Constant))
        return std::make_unique<Constant>(k_functions.at(this->function)(
            dynamic_cast<Constant &>(*param).value
        ));

    return std::make_unique<Function>(this->function, std::move(param));
}

Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

Term::Term(double const coefficient, OwnedToken &&base, OwnedToken &&power)
    : coefficient(coefficient), base(std::move(base)), power(std::move(power)) {
}

Term::Term(OwnedToken &&base, OwnedToken &&power)
    : base(std::move(base)), power(std::move(power)) {}

OwnedToken Term::clone() const {
    return std::make_unique<Term>(
        this->coefficient.value, this->base->clone(), this->power->clone()
    );
}

bool Term::is_dependent_on(Variable const &variable) const {
    if (typeid(*this->base) == typeid(Constant) &&
        typeid(*this->power) == typeid(Constant))
        return false;

    return dynamic_cast<Dependent &>(*this->base).is_dependent_on(variable) ||
           dynamic_cast<Dependent &>(*this->power).is_dependent_on(variable);
}

OwnedToken Term::at(std::map<Variable, SharedToken> const &values) const {
    auto const clone = this->clone();
    auto &term = dynamic_cast<Term &>(*clone);
    term.base = dynamic_cast<Evaluatable &>(*term.base).at(values);
    term.power = dynamic_cast<Evaluatable &>(*term.power).at(values);

    return term.simplified();
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
            if (dynamic_cast<Constant &>(*this->power).value != 1) {
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

OwnedToken Terms::clone() const {
    auto terms = std::make_unique<Terms>();
    terms->coefficient = this->coefficient;

    for (auto const &term : this->terms)
        terms->add_term(term->clone());

    return terms;
}

void Terms::add_term(OwnedToken &&token) {
    auto const &term_type = typeid(*token);

    if (term_type == typeid(Constant)) {
        this->coefficient.value *= dynamic_cast<Constant &>(*token).value;

        return;
    }

    if (term_type == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);
        this->coefficient.value *= term.coefficient.value;
        term.coefficient.value = 1;

        this->terms.push_back(std::move(token));

        return;
    }

    if (term_type == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);
        this->coefficient.value *= terms.coefficient.value;
        terms.coefficient.value = 1;

        for (OwnedToken &term : terms.terms)
            this->add_term(std::move(term));

        return;
    }

    this->terms.push_back(std::move(token));
}

bool Terms::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->terms,
        [variable](OwnedToken const &token) -> bool {
            return typeid(*token) != typeid(Constant) &&
                   dynamic_cast<Dependent &>(*token).is_dependent_on(variable);
        }
    );
}

OwnedToken Terms::at(std::map<Variable, SharedToken> const &values) const {
    auto const clone = this->clone();
    auto &terms = dynamic_cast<Terms &>(*clone);

    for (auto &term : terms.terms)
        term = dynamic_cast<Evaluatable &>(*term).at(values);

    return terms.simplified();
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

OwnedToken Expression::clone() const {
    auto expression = std::make_unique<Expression>();

    for (auto const &token : this->tokens)
        expression->add_token(token->clone());

    return expression;
}

void Expression::add_token(OwnedToken &&token) {
    this->tokens.push_back(std::move(token));
}

OwnedToken Expression::pop_token() {
    OwnedToken token = std::move(this->tokens.back());

    this->tokens.pop_back();

    return token;
}

bool Expression::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->tokens,
        [variable](OwnedToken const &token) -> bool {
            return typeid(*token) != typeid(Constant) &&
                   typeid(*token) != typeid(Operation) &&
                   dynamic_cast<Dependent &>(*token).is_dependent_on(variable);
        }
    );
}

OwnedToken Expression::at(std::map<Variable, SharedToken> const &values) const {
    auto const clone = this->clone();
    auto &expression = dynamic_cast<Expression &>(*clone);

    for (auto &term : expression.tokens)
        if (typeid(*term) != typeid(Operation))
            term = dynamic_cast<Evaluatable &>(*term).at(values);

    return expression.simplified();
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << static_cast<std::string>(*this->tokens[0]);
    } else {
        result << '(';

        for (OwnedToken const &token : this->tokens)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

OwnedToken tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    auto terms = std::make_unique<Terms>();

    for (int i = 0; i < expression.size(); ++i) {
        std::int32_t const copy = i;

        OwnedToken token = get_next_token(expression, i);

        if (!token)
            continue;

        if (typeid(*token) == typeid(Operation)) {
            auto &operation = dynamic_cast<Operation &>(*token);

            if (operation.operation == Operation::add ||
                operation.operation == Operation::sub) {
                if (!terms->terms.empty())
                    result.add_token(std::move(terms));

                else if (copy != 0)
                    result.add_token(
                        std::make_unique<Constant>(terms->coefficient)
                    );

                terms = std::make_unique<Terms>();
                result.add_token(std::move(token));

                continue;
            }

            if (operation.operation == Operation::mul) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value *=
                        dynamic_cast<Constant &>(*next).value;
                } else {
                    terms->add_term(std::move(next));
                }

                continue;
            }

            if (operation.operation == Operation::div) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value /=
                        dynamic_cast<Constant &>(*next).value;
                } else {
                    terms->add_term(std::make_unique<Term>(
                        1, std::move(next), std::make_unique<Constant>(-1)
                    ));
                }

                continue;
            }

            std::vector<OwnedToken> powers;

            OwnedToken next;

            while (true) {
                next = get_next_token(expression, ++i);

                if (!next)
                    break;

                if (typeid(*next) == typeid(Operation)) {
                    operation = dynamic_cast<Operation &>(*next);

                    if (operation.operation == Operation::pow)
                        continue;

                    break;
                }

                if (typeid(*next) == typeid(Term)) {
                    powers.push_back(std::move(next));
                } else {
                    powers.push_back(std::make_unique<Term>(
                        1, std::move(next), std::make_unique<Constant>(1)
                    ));
                }
            }

            OwnedToken power = std::make_unique<Constant>(1);

            for (OwnedToken &p : powers | std::views::reverse) {
                power =
                    std::make_unique<Term>(1, std::move(p), std::move(power));
            }

            terms->terms.back() = std::make_unique<Term>(
                1, std::move(terms->terms.back()), std::move(power)
            );

            if (next) {
                --i;
            }

            continue;
        }

        if (typeid(*token) == typeid(Constant)) {
            terms->terms.push_back(std::move(token));
        } else {
            terms->add_term(std::move(token));
        }
    }

    if (!terms->terms.empty())
        result.add_token(std::move(terms));

    else
        result.add_token(terms->coefficient.clone());

    return result.simplified();
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}