#include "expression.hpp"

#include <algorithm>
#include <iostream>
#include <cmath>

#define DEFINE_OPERATOR(op)						\
	Expression& Expression::operator op## =	(const Expression &e)	\
	{											\
		addFunction(findFunction(#op, Function::Type::INFIX), e);	\
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

const std::string Expression::m_whitespaces = " \t\n";
const Functions Expression::m_operators = {
	Function("+", 10, [=](const Args &a){return a[0] + a[1];}, true),
	Function("-", 10, [&](const Args &a){return a[0] - a[1];}, false),
	Function("*", 20, [&](const Args &a){return a[0] * a[1];}, true),
	Function("/", 20, [&](const Args &a){return a[0] / a[1];}, false),
	Function("-", 40, [&](const Args &a){return -a[0];}, Function::Type::PREFIX)};
const Functions Expression::m_functions = {
	Function("abs", [&](const Args &a){return abs(a[0]);}),
	Function("ceil", [&](const Args &a){return ceil(a[0]);}),
	Function("floor", [&](const Args &a){return floor(a[0]);}),
	Function("max", [&](const Args &a){return std::max(a[0], a[1]);}, 2),
	Function("min", [&](const Args &a){return std::min(a[0], a[1]);}, 2),

	Function("sin", [&](const Args &a){return sin(a[0]);}),
	Function("cos", [&](const Args &a){return cos(a[0]);}),
	Function("tan", [&](const Args &a){return tan(a[0]);}),
	Function("ctg", [&](const Args &a){return 1.0 / tan(a[0]);}),
	Function("asin", [&](const Args &a){return asin(a[0]);}),
	Function("acos", [&](const Args &a){return acos(a[0]);}),
	Function("atan", [&](const Args &a){return atan(a[0]);}),
	Function("atan2", [&](const Args &a){return atan2(a[0], a[1]);}, 2),

	Function("cosh", [&](const Args &a){return cosh(a[0]);}),
	Function("sinh", [&](const Args &a){return sinh(a[0]);}),
	Function("tanh", [&](const Args &a){return tanh(a[0]);}),
	Function("ctgh", [&](const Args &a){return 1.0 / (a[0]);}),
	Function("acosh", [&](const Args &a){return acosh(a[0]);}),
	Function("asinh", [&](const Args &a){return asinh(a[0]);}),
	Function("atanh", [&](const Args &a){return atanh(a[0]);}),
	Function("actgh", [&](const Args &a){return atanh(1.0 /a[0]);})};

Expression::Expression(const std::string &s) :
	m_root(nullptr)
{
	ExpressionParserSettings set(m_whitespaces, m_operators, m_functions, m_variables);
	ExpressionParser p(set);
	m_root = p.parse(s);
	if(m_root) {
		cout << "You entered: " << endl;
		m_root->print();
		cout << endl;
	}
	// for(auto &i : m_variables) {
	// 	cout << i.first << " = ";
	// 	std::cin >> i.second;
	// }
	// cout << m_root->eval() << endl;
}

Expression::Expression(const Expression &e) :
	m_root(nullptr),
	m_variables(e.m_variables)
{
	m_root = new Cell(*e.m_root);
}

Expression& Expression::operator=(const Expression &e)
{
	if(m_root != nullptr) {
		delete m_root;
	}
	new Cell(*e.m_root);
	m_variables = e.m_variables;
	return *this;
}

bool Expression::operator==(const Expression &e) const
{
	return (m_root == e.m_root) || (*m_root == *e.m_root);
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
	std::vector <Cell*> curcell;
	curcell.push_back(e.m_root);
	while(curcell[curcell.size() - 1]->type == Cell::Type::FUNCTION) {
		curcell.push_back(curcell[curcell.size() - 1]->func.args[0]);
	}
	bool tmp;
	return m_root->isSubExpression(curcell, tmp);
}

Functions::const_iterator Expression::findFunction(const std::string &name, Function::Type type)
{
	Functions::const_iterator res = m_operators.end();
	for(auto i = m_operators.begin(); i != m_operators.end(); ++i) {
		if((type == i->type) && (name == i->name)) {
			res = i;
		}
	}
	return res;
}

void Expression::addFunction(const Functions::const_iterator &f, const Expression &e)
{
	Cell *tmp = m_root;
	Cell *arg2 = new Cell(*e.m_root);
	m_root = new Cell();
	m_root->type = Cell::Type::FUNCTION;
	m_root->func.iter = f;
	m_root->func.args.push_back(tmp);
	m_root->func.args.push_back(arg2);
}
