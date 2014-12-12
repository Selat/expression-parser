#include "expression.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

using std::cout;
using std::endl;

const std::string ExpressionParser::m_whitespaces = " \t\n";
const Functions ExpressionParser::m_operators = {
	Function("+", 10, [&](Args &a){return a[0] + a[1];}),
	Function("-", 10, [&](Args &a){return a[0] - a[1];}),
	Function("*", 20, [&](Args &a){return a[0] * a[1];}),
	Function("/", 20, [&](Args &a){return a[0] / a[1];})};
const Functions ExpressionParser::m_functions = {
	Function("abs", [&](Args &a){return abs(a[0]);}),
	Function("ceil", [&](Args &a){return ceil(a[0]);}),
	Function("floor", [&](Args &a){return floor(a[0]);}),
	Function("max", [&](Args &a){return std::max(a[0], a[1]);}, 2),
	Function("min", [&](Args &a){return std::min(a[0], a[1]);}, 2),

	Function("sin", [&](Args &a){return sin(a[0]);}),
	Function("cos", [&](Args &a){return cos(a[0]);}),
	Function("tan", [&](Args &a){return tan(a[0]);}),
	Function("ctg", [&](Args &a){return 1.0 / tan(a[0]);}),
	Function("asin", [&](Args &a){return asin(a[0]);}),
	Function("acos", [&](Args &a){return acos(a[0]);}),
	Function("atan", [&](Args &a){return atan(a[0]);}),
	Function("atan2", [&](Args &a){return atan2(a[0], a[1]);}, 2),

	Function("cosh", [&](Args &a){return cosh(a[0]);}),
	Function("sinh", [&](Args &a){return sinh(a[0]);}),
	Function("tanh", [&](Args &a){return tanh(a[0]);}),
	Function("ctgh", [&](Args &a){return 1.0 / (a[0]);}),
	Function("acosh", [&](Args &a){return acosh(a[0]);}),
	Function("asinh", [&](Args &a){return asinh(a[0]);}),
	Function("atanh", [&](Args &a){return atanh(a[0]);}),
	Function("actgh", [&](Args &a){return atanh(1.0 /a[0]);})};

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

ExpressionParser::ExpressionParser(std::map <std::string, double> &variables) :
	m_variables(variables), real_shift(0), is_recursive_call(false)
{
}

ExpressionParser::ExpressionParser(std::map <std::string, double> &variables,
                                   const std::string &_real_s, size_t shift) :
	m_variables(variables), real_s(_real_s),
	real_shift(shift), is_recursive_call(true)
{
}

Expression::Expression(const std::string &s) :
	m_root(nullptr)
{
	ExpressionParser p(m_variables);
	m_root = p.parse(s);
	m_root->print();
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
		} else if(isalpha(s[id])) {
			parseVariable(s);
		} else {
			throwError("Unrecognised token: ", id);
		}
	}
	if(curcell->type == Cell::Type::NONE) {
		throwError("Right argument for operator not found: ", last_op_id);
	}
	return root;
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
	ExpressionParser p(m_variables, real_s, real_shift + id);
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
	auto f = findItem(s, id, m_operators);
	if(!is_prev_num) {
		throwError("Expected value before the operator: ", id);
	}
	is_prev_num = false;
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
	tcell->type = Cell::Type::FUNCTION;
	tcell->func.iter = f;
	tcell->func.args.push_back(curcell);
	curcell = new Cell();
	tcell->func.args.push_back(curcell);
	parents.push_back(tcell);
	id += f->name.length();
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
			ExpressionParser p(m_variables, real_s, real_shift + prev_id);
			curcell->func.args.push_back(p.parse(s.substr(prev_id, cid - prev_id)));
			prev_id = cid + 1;
		}
	}
	ExpressionParser p(m_variables, real_s, real_shift + prev_id);
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

Functions::const_iterator ExpressionParser::findItem(const std::string &s, size_t id, const Functions &coll)
{
	Functions::const_iterator res = coll.end();
	for(auto i = coll.begin(); i != coll.end(); ++i) {
		if((s.length() - id >= i->name.length())
		   && (s.substr(id, i->name.length()) == i->name)
		   && ((res == coll.end()) || (res->name.length() < i->name.length()))) {
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
