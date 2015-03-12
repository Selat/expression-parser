#include "expression.hpp"

#include <algorithm>
#include <iostream>
#include <cmath>

#define DEFINE_OPERATOR(op)						\
	Expression& Expression::operator op## =	(const Expression &e)	\
	{											\
		addFunction(findFunction(#op, Function<int>::Type::INFIX), e); \
		return *this;							\
	}											\

#define DEFINE_OPERATORV(op)					\
	Expression Expression::operator op(const Expression &e) const	\
	{											\
		Expression	res(*this);					\
		res op## = e;							\
		return res;								\
	}

using std::cout;
using std::endl;

namespace {
const Functions<int> operators = {
	Function<int>("+", 10, [](const Args<int> &a){return a[0] + a[1];}, true),
	Function<int>("-", 10, [](const Args<int> &a){return a[0] - a[1];}, false),
	Function<int>("*", 20, [](const Args<int> &a){return a[0] * a[1];}, true),
	Function<int>("/", 20, [](const Args<int> &a){return a[0] / a[1];}, false),
	Function<int>("-", 40, [](const Args<int> &a){return -a[0];}, Function<int>::Type::PREFIX)};
const Functions<int> functions = {
	Function<int>("abs", [](const Args<int> &a){return std::abs(a[0]);}),
	Function<int>("ceil", [](const Args<int> &a){return ceil(a[0]);}),
	Function<int>("floor", [](const Args<int> &a){return floor(a[0]);}),
	Function<int>("max", [](const Args<int> &a){return std::max(a[0], a[1]);}, 2),
	Function<int>("min", [](const Args<int> &a){return std::min(a[0], a[1]);}, 2),

	Function<int>("sin", [](const Args<int> &a){return sin(a[0]);}),
	Function<int>("cos", [](const Args<int> &a){return cos(a[0]);}),
	Function<int>("tan", [](const Args<int> &a){return tan(a[0]);}),
	Function<int>("ctg", [](const Args<int> &a){return 1.0 / tan(a[0]);}),
	Function<int>("asin", [](const Args<int> &a){return asin(a[0]);}),
	Function<int>("acos", [](const Args<int> &a){return acos(a[0]);}),
	Function<int>("atan", [](const Args<int> &a){return atan(a[0]);}),
	Function<int>("atan2", [](const Args<int> &a){return atan2(a[0], a[1]);}, 2),

	Function<int>("cosh", [](const Args<int> &a){return cosh(a[0]);}),
	Function<int>("sinh", [](const Args<int> &a){return sinh(a[0]);}),
	Function<int>("tanh", [](const Args<int> &a){return tanh(a[0]);}),
	Function<int>("ctgh", [](const Args<int> &a){return 1.0 / (a[0]);}),
	Function<int>("acosh", [](const Args<int> &a){return acosh(a[0]);}),
	Function<int>("asinh", [](const Args<int> &a){return asinh(a[0]);}),
	Function<int>("atanh", [](const Args<int> &a){return atanh(a[0]);}),
	Function<int>("actgh", [](const Args<int> &a){return atanh(1.0 /a[0]);})};
}

Expression::Expression(const std::string &s) :
	m_root(nullptr)
{
	ExpressionParserSettings <int> set(operators, functions, m_varnames);
	set.regex_whitespace = std::regex("^[[:space:]]+");
	set.regex_constant = std::regex("^[[:digit:]]+");
	set.regex_parenthesis_begin = std::regex("^\\(");
	set.regex_parenthesis_end = std::regex("^\\)");
	set.regex_variable = std::regex("^[[:alpha:]][[:alnum:]]*");
	set.regex_function_begin = std::regex("^[[:alpha:]][[:alnum:]]*[[:space:]]*\\(");
	set.regex_function_end = std::regex("^\\)");
	set.regex_func_args_separator = std::regex("^,");
	ExpressionParser <int> p(set, s);
	m_root = p.parse();
	if(m_root) {
		cout << "You entered: " << endl;
		m_root->print();
		cout << endl;
		for(const auto &s : m_varnames) {
			m_variables[s] = 0.0;
		}

		cout << "Expression: " << endl;
		for(auto it = m_root->begin(); it != m_root->end(); ++it) {
			it->print();
			cout << ", ";
		}
	}
}

Expression::Expression(const Expression &e) :
	m_root(nullptr),
	m_variables(e.m_variables)
{
	m_root = new Cell <int>(*e.m_root);
}

Expression& Expression::operator=(const Expression &e)
{
	if(this != &e) {
		if(m_root != nullptr) {
			delete m_root;
		}
		m_root = new Cell <int>(*e.m_root);
		m_variables = e.m_variables;
		m_varnames = e.m_varnames;
	}
	return *this;
}

bool Expression::operator==(const Expression &e) const
{
	return (m_root == e.m_root) || ((m_root != nullptr) && (e.m_root != nullptr) && (*m_root == *e.m_root));
}

bool Expression::operator!=(const Expression &e) const
{
	return !(*this == e);
}

DEFINE_OPERATOR(+);
DEFINE_OPERATOR(-);
DEFINE_OPERATOR(*);
DEFINE_OPERATOR(/);

DEFINE_OPERATORV(+);
DEFINE_OPERATORV(-);
DEFINE_OPERATORV(*);
DEFINE_OPERATORV(/);

bool Expression::isSubExpression(const Expression &e) const
{
	std::vector <Cell <int>*> curcell;
	curcell.push_back(e.m_root);
	while(curcell[curcell.size() - 1]->type == Cell <int>::Type::FUNCTION) {
		curcell.push_back(curcell[curcell.size() - 1]->func.args[0]);
	}
	bool tmp;
	return m_root->isSubExpression(curcell, tmp);
}

std::map <std::string, int>& Expression::variables()
{
	return m_variables;
}

int Expression::getVar(size_t id) const
{
	if(id >= m_varnames.size()) {
		throw ExpressionException("Index out of range");
	}
	return m_variables.at(m_varnames[id]);
}

int Expression::getVar(const std::string &name) const
{
	auto it = m_variables.find(name);
	if(it == m_variables.end()) {
		throw ExpressionException("Undefined variable: " + name);
	}
	return m_variables.at(name);
}

void Expression::setVar(size_t id, int val)
{
	m_variables[m_varnames[id]] = val;
}

void Expression::setVar(const std::string &name, int val)
{
	auto it = m_variables.find(name);
	if(it == m_variables.end()) {
		throw ExpressionException("Undefined variable: " + name);
	}
	it->second = val;
}

Functions<int>::const_iterator Expression::findFunction(const std::string &name, Function<int>::Type type)
{
	auto res = operators.end();
	for(auto i = operators.begin(); i != operators.end(); ++i) {
			if((type == i->type) && (name == i->name)) {
			res = i;
		}
	}
	return res;
}

void Expression::addFunction(const Functions<int>::const_iterator &f, const Expression &e)
{
	Cell <int> *tmp = m_root;
	Cell <int> *arg2 = new Cell <int>(*e.m_root);
	m_root = new Cell <int>();
	m_root->type = Cell <int>::Type::FUNCTION;
	m_root->func.iter = f;
	m_root->func.args.push_back(tmp);
	m_root->func.args.push_back(arg2);
}

void Expression::print()
{
	m_root->print();
}


int Expression::eval()
{
	return m_root->eval(m_variables);
}
