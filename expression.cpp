#include "expression.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

using std::cout;
using std::endl;

const std::string ExpressionParser::m_whitespaces = " \t\n";
const Functions Expression::m_operators = {
	Function("+", 10, [=](const Args &a){return a[0] + a[1];}, true),
	Function("-", 10, [&](const Args &a){return a[0] - a[1];}, false),
	Function("*", 20, [&](const Args &a){return a[0] * a[1];}, true),
	Function("/", 20, [&](const Args &a){return a[0] / a[1];}, false),
	Function("-", 40, [&](const Args &a){return a[0] * a[1];}, Function::Type::PREFIX)};
const Functions ExpressionParser::m_functions = {
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

void Cell::print()
{
	if(type == Type::FUNCTION) {
		cout << "(";
		cout << func.iter->name;
		for(const auto &i : func.args) {
			cout << " ";
			i->print();
		}
		cout << ")";
	} else if(type == Type::VARIABLE) {
		cout << var.iter->first;
	} else if(type == Type::NUMBER) {
		cout << val;
	}
}

double Cell::eval()
{
	switch(type) {
	case Type::FUNCTION:
	{
		Args args;
		for(auto i : func.args) {
			args.push_back(i->eval());
		}
		return func.iter->func(args);
		break;
	}
	case Type::VARIABLE:
	{
		return var.iter->second;
		break;
	}
	case Type::NUMBER:
	{
		return val;
		break;
	}
	default:
		throw ExpressionException("Attempt to evaluate cell of type \"NONE\"");
	}
}

ExpressionParser::ExpressionParser(std::map <std::string, double> &variables,
                                   const Functions &operators) :
	m_operators(operators), m_variables(variables), real_shift(0), is_recursive_call(false)
{
}

ExpressionParser::ExpressionParser(std::map <std::string, double> &variables,
                                   const Functions &operators,
                                   const std::string &_real_s, size_t shift) :
	m_operators(operators), m_variables(variables), real_s(_real_s),
	real_shift(shift), is_recursive_call(true)
{
}

Expression::Expression(const std::string &s) :
	m_root(nullptr)
{
	ExpressionParser p(m_variables, m_operators);
	m_root = p.parse(s);
	cout << "You entered: " << endl;
	m_root->print();
	cout << endl;
	for(auto &i : m_variables) {
		cout << i.first << " = ";
		std::cin >> i.second;
	}
	cout << m_root->eval() << endl;
}

Cell* ExpressionParser::parse(const std::string &s)
{
	if(!is_recursive_call) {
		real_s = s;
	}
	id = 0;
	root = new Cell();
	curcell = root;
	is_prev_num = false;
	real_s = real_s;
	last_op_id = 0;

	while(id < s.length()) {
		parseNextToken(s);
	}
	if(curcell->type == Cell::Type::NONE) {
		throwError("Right argument for operator not found: ", last_op_id);
	}
	return root;
}

void ExpressionParser::parseNextToken(const std::string &s)
{
	if(isWhitespace(s[id])) {
		++id;
	} else if(!is_prev_num && isConstant(s, id)) {
		parseNumber(s);
	} else if(isOperator(s, id)) {
		last_op_id = id;
		parseOperator(s);
	} else if(isFunction(s, id)) {
		parseFunction(s);
	} else if(isParenthesis(s[id])) {
		parseParenthesis(s);
	} else if(isVarBeginning(s[id])) {
		parseVariable(s);
	} else {
		throwError("Unrecognised token: ", id);
	}
}

void ExpressionParser::parseVariable(const std::string &s)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	int start = id;
	id = seekVar(s, id);
	std::string varname = s.substr(start, id - start);
	m_variables[varname] = 0.0;
	curcell->type = Cell::Type::VARIABLE;
	curcell->var.iter = m_variables.find(varname);
	is_prev_num = true;
}

void ExpressionParser::parseNumber(const std::string &s)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	int start = id;
	if(s[id] == '-') {
		++id;
	}
	id = seekNumber(s, id);
	std::stringstream ss(s.substr(start, id - start));
	double val;
	ss >> val;
	curcell->type = Cell::Type::NUMBER;
	curcell->val = val;
	is_prev_num = true;
}

void ExpressionParser::parseParenthesis(const std::string &s)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	int end = findMatchingParenthesis(s, id);
	++id;
	ExpressionParser p(m_variables, m_operators, real_s, real_shift + id);
	Cell *tcell = p.parse(s.substr(id, end - id));
	if(parents.empty()) {
		delete root;
		root = curcell = tcell;
	} else {
		delete curcell;
		parents[parents.size() - 1]->func.args[1] = tcell;
	}
	id = end + 1;
	is_prev_num = true;
}

void ExpressionParser::parseOperator(const std::string &s)
{
	Functions::const_iterator f;
	if(!is_prev_num) {
		// We have to parse it as prefix operator because previous token is some operator.
		f = findItem(s, id, m_operators, Function::Type::PREFIX);
		if(f != m_operators.end()) {
			// Read argument of the operator.
			size_t start = id;
			id += f->name.length();
			while((id < s.length()) && isWhitespace(s[id])) ++id;
			if(id < s.length()) {
				parseNextToken(s);
			} else {
				throwError("Argument for prefix operator isn't found: ", start);
			}
			is_prev_num = true;
		} else {
			throwError("Expected prefix operator: ", id);
		}
	} else {
		// We have to parse it as infix/postfix operator because previous token is some value.
		// First argument for these operators is already stored for us in curcell, so we don't have to do anything.
		// Check next token ("value" - for infix operator, "operator" - for postfix operator) to choose right operator.
		size_t start = id;
		++id;
		while((id < s.length()) && isWhitespace(s[id])) ++id;
		bool second_val = (id < s.length()) || isConstant(s, id) || isFunction(s, id) || isVarBeginning(s[id]) || (s[id] == '(');
		if((id < s.length()) && second_val) {
			id = start;
			f = findItem(s, id, m_operators, Function::Type::INFIX);
			if(f != m_operators.end()) {
				// Restore value of
				id += f->name.length();
				is_prev_num = false;
			} else {
				throwError("Expected infix operator: ", start);
			}
		} else {
			id = start;
			f = findItem(s, id, m_operators, Function::Type::POSTFIX);
			if(f != m_operators.end()) {
				// Restore value of id
				is_prev_num = true;
				cout << "fucking found!" << endl;
				id += f->name.length();
			} else {
				throwError("Expected postfix operator: ", id);
			}
		}
	}
	Cell *tcell = new Cell();
	if(curcell == root) {
		root = tcell;
	}
	if(!parents.empty()) {
		int id = parents.size() - 1;
		if((id >= 0) && (f->precedence <= parents[id]->func.iter->precedence)) {
			--id;
		}
		while((id >= 0) && (f->precedence <= parents[id]->func.iter->precedence)) {
			--id;
			parents.pop_back();
		}
		if(id >= 0) {
			curcell = parents[id]->func.args[1];
			parents[id]->func.args[1] = tcell;
		} else {
			curcell = root;
			root = tcell;
			parents.pop_back();
		}
	}
	{
		Args args;
		args.push_back(1);
		args.push_back(1);
		std::cout << "Test: " << f->func(args) << std::endl;
	}
	tcell->type = Cell::Type::FUNCTION;
	tcell->func.iter = f;
	tcell->func.args.push_back(curcell);
	if(f->type == Function::Type::INFIX) {
		curcell = new Cell();
		tcell->func.args.push_back(curcell);
	}
	parents.push_back(tcell);
}

void ExpressionParser::parseFunction(const std::string &s)
{
	auto f = findItem(s, id, m_functions);
	if(f == m_functions.end()) {
		throwError("Undefined function: ", id);
	}
	size_t cid = id + f->name.length();
	while((cid < s.length()) && isWhitespace(s[cid])) ++cid;
	if((cid >= s.length()) || (s[cid] != '(')) {
		throwError("Expected list of parameters after the name of the function: ", cid);
	}
	size_t end = findMatchingParenthesis(s, cid);
	curcell->type = Cell::Type::FUNCTION;
	curcell->func.iter = f;
	int level = 0;
	size_t prev_id = cid + 1;
	for(++cid; cid < end; ++cid) {
		if(s[cid] == '(') {
			++level;
		} else if(s[cid] == ')') {
			--level;
		}
		// We found the next argument of the function.
		if((level == 0) && (s[cid] == ',')) {
			ExpressionParser p(m_variables, m_operators, real_s, real_shift + prev_id);
			curcell->func.args.push_back(p.parse(s.substr(prev_id, cid - prev_id)));
			prev_id = cid + 1;
		}
	}
	ExpressionParser p(m_variables, m_operators, real_s, real_shift + prev_id);
	curcell->func.args.push_back(p.parse(s.substr(prev_id, cid - prev_id)));
	if(curcell->func.args.size() > f->args_num) {
		throwError("Invalid number of arguments: ", cid);
	}
	id = end + 1;
	is_prev_num = true;
}

void ExpressionParser::throwError(const std::string &msg, size_t id)
{
	std::stringstream ss;
	ss << msg << endl;
	ss << real_s << endl;
	int n = id + real_shift;
	for(int i = 0; i < n; ++i) {
		ss << " ";
	}
	ss << "^" << endl;
	throw ExpressionException(ss.str());
}

size_t ExpressionParser::findMatchingParenthesis(const std::string &s, size_t id)
{
	int c = 1;
	size_t start_id = id;
	++id;
	while((id < s.length()) && (c > 0)) {
		if(s[id] == '(') {
			++c;
		} else if(s[id] == ')') {
			--c;
		}
		++id;
	}
	if(c == 0) {
		return id - 1;
	} else {
		throwError("Mismatched parentheses: ", start_id);
		return 0;
	}
}

Functions::const_iterator ExpressionParser::findItem(const std::string &s, size_t id, const Functions &coll,
                                                     Function::Type type)
{
	Functions::const_iterator res = coll.end();
	for(auto i = coll.begin(); i != coll.end(); ++i) {
		if((s.length() - id >= i->name.length())
		   && (s.substr(id, i->name.length()) == i->name)
		   && ((res == coll.end()) || (res->name.length() < i->name.length()))
		   && ((type == Function::Type::NONE) || (type == i->type))) {
			res = i;
		}
	}
	return res;
}

bool ExpressionParser::isWhitespace(char c)
{
	return (m_whitespaces.find(c) != std::string::npos);
}

bool ExpressionParser::isParenthesis(char c)
{
	return (c == '(') || (c == ')') || (c == '[') || (c == ']');
}

bool ExpressionParser::isVarBeginning(char c)
{
	return isalpha(c) || isdigit(c) || (c == '_');
}

bool ExpressionParser::isOperator(const std::string &s, size_t id)
{
	return findItem(s, id, m_operators) != m_operators.end();
}

bool ExpressionParser::isFunction(const std::string &s, size_t id)
{
	// Function should be of the following form:
	// TODO: fix errors in regexp:
	// "^[[:alpha:]][[:alnum:]]*[[:space:]]*("
	size_t start = id;
	while((id < s.length()) && (isalpha(s[id]) || (isdigit(s[id])))) ++id;
	if((id < s.length()) && (id > start)) {
		while((id < s.length()) && isWhitespace(s[id])) ++id;
		return (id < s.length()) && (s[id] == '(');
	}
	return false;
}

bool ExpressionParser::isConstant(const std::string &s, size_t id)
{
	return isdigit(s[id]) || ((s[id] == '-') && (id + 1 < s.length()) && isdigit(s[id + 1]));
}

int ExpressionParser::seekVar(const std::string &s, size_t id)
{
	while((id < s.length()) && (isalpha(s[id]) || isdigit(s[id]) || (s[id] == '_'))) ++id;
	return id;
}

int ExpressionParser::seekNumber(const std::string &s, size_t id)
{
	bool found_dot = false;
	while((id < s.length()) && ((isdigit(s[id]) || (s[id] == '.')))) {
		if(s[id] == '.') {
			if(found_dot) {
				throwError("Found second dot in a real number: ", id);
			}
			found_dot = true;
		}
		++id;
	}
	return id;
}
