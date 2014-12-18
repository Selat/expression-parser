#include "expression_parser.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

using std::cout;
using std::endl;

ExpressionParser::ExpressionParser(ExpressionParserSettings &_settings,
								   const std::string &_str) :
	settings(_settings), str(_str)
{
}

Cell* ExpressionParser::parse()
{
	if(str.length() == 0) {
		return nullptr;
	}
	size_t id = 0;
	Cell *res = _parse(id);
	if(id < str.length()) {
		if(str[id] == ')') {
			throwError("Mismatched parentheses: ", id);
		} else {
			throwError("Something wrong here: ", id);
		}
	}
	res->sort();
	return res;
}

Cell* ExpressionParser::_parse(size_t &tid)
{
	id = tid;
	root = new Cell();
	curcell = root;
	is_prev_num = false;
	last_op_id = 0;
	while(id < str.length()) {
		if((str[id] == ',') || (str[id] == ')')) {
			break;
		}
		parseNextToken();
	}
	if(curcell->type == Cell::Type::NONE) {
		throwError("Right argument for operator not found: ", last_op_id);
	}
	tid = id;
	return root;
}

void ExpressionParser::parseNextToken()
{
	if(isWhitespace(str[id])) {
		++id;
	} else if(!is_prev_num && isConstant(id)) {
		parseNumber();
	} else if(isOperator(id)) {
		last_op_id = id;
		parseOperator();
	} else if(isFunction(id)) {
		parseFunction();
	} else if(isParenthesis(str[id])) {
		parseParenthesis();
	} else if(isVarBeginning(str[id])) {
		parseVariable();
	} else {
		throwError("Unrecognised token: ", id);
	}
}

void ExpressionParser::parseVariable()
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	int start = id;
	id = seekVar(id);
	std::string varname = str.substr(start, id - start);
	curcell->type = Cell::Type::VARIABLE;
	curcell->var.name = varname;
	is_prev_num = true;

	bool exist = false;
	for(const auto &i : settings.variables) {
		if(i == varname) {
			exist = true;
			break;
		}
	}
	if(!exist) {
		settings.variables.push_back(varname);
	}
}

void ExpressionParser::parseNumber()
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	int start = id;
	if(str[id] == '-') {
		++id;
	}
	id = seekNumber(id);
	std::stringstream ss(str.substr(start, id - start));
	double val;
	ss >> val;
	curcell->type = Cell::Type::NUMBER;
	curcell->val = val;
	is_prev_num = true;
}

void ExpressionParser::parseParenthesis()
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", id);
	}
	size_t start_id = id;
	++id;
	ExpressionParser p(settings, str);
	Cell *tcell = p._parse(id);
	if(id == str.length()) {
		throwError("Mismatched parentheses: ", start_id);
	}
	++id;
	if(parents.empty()) {
		delete root;
		root = curcell = tcell;
	} else {
		delete curcell;
		parents[parents.size() - 1]->func.args[1] = tcell;
	}
	is_prev_num = true;
}

void ExpressionParser::parseOperator()
{
	Functions::const_iterator f;
	if(!is_prev_num) {
		// We have to parse it as prefix operator because previous token is some operator.
		f = findItem(id, settings.operators, Function::Type::PREFIX);
		if(f != settings.operators.end()) {
			// Read argument of the operator.
			size_t start = id;
			id += f->name.length();
			while((id < str.length()) && isWhitespace(str[id])) ++id;
			if(id < str.length()) {
				parseNextToken();
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
		while((id < str.length()) && isWhitespace(str[id])) ++id;
		bool second_val = (id < str.length()) || isConstant(id) || isFunction(id) || isVarBeginning(str[id]) || (str[id] == '(');
		if((id < str.length()) && second_val) {
			id = start;
			f = findItem(id, settings.operators, Function::Type::INFIX);
			if(f != settings.operators.end()) {
				// Restore value of
				id += f->name.length();
				is_prev_num = false;
			} else {
				throwError("Expected infix operator: ", start);
			}
		} else {
			id = start;
			f = findItem(id, settings.operators, Function::Type::POSTFIX);
			if(f != settings.operators.end()) {
				// Restore value of id
				is_prev_num = true;
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
	tcell->type = Cell::Type::FUNCTION;
	tcell->func.iter = f;
	tcell->func.args.push_back(curcell);
	if(f->type == Function::Type::INFIX) {
		curcell = new Cell();
		tcell->func.args.push_back(curcell);
	}
	parents.push_back(tcell);
}

void ExpressionParser::parseFunction()
{
	auto f = findItem(id, settings.functions);
	if(f == settings.functions.end()) {
		throwError("Undefined function: ", id);
	}
	id += f->name.length();
	size_t func_name_id = id;
	skipWhitespaces();

	if((id >= str.length()) || (str[id] != '(')) {
		throwError("Expected list of parameters after the name of the function: ", id);
	}
	curcell->type = Cell::Type::FUNCTION;
	curcell->func.iter = f;
	size_t prev_id = id + 1;
	while(prev_id < str.length()) {
		ExpressionParser p(settings, str);
		curcell->func.args.push_back(p._parse(prev_id));
		if(str[prev_id] == ')') {
			++prev_id;
			break;
		}
		++prev_id;
	}
	if(curcell->func.args.size() != f->args_num) {
		throwError("Invalid number of arguments: ", func_name_id);
	}
	id = prev_id;
	is_prev_num = true;
}

void ExpressionParser::throwError(const std::string &msg, size_t id) const
{
	std::stringstream ss;
	ss << msg << endl;
	ss << str << endl;
	int n = id;
	for(int i = 0; i < n; ++i) {
		ss << " ";
	}
	ss << "^" << endl;
	throw ExpressionParserException(ss.str());
}

Functions::const_iterator ExpressionParser::findItem(size_t id, const Functions &coll,
                                                     Function::Type type)
{
	Functions::const_iterator res = coll.end();
	for(auto i = coll.begin(); i != coll.end(); ++i) {
		if((str.length() - id >= i->name.length())
		   && (str.substr(id, i->name.length()) == i->name)
		   && ((res == coll.end()) || (res->name.length() < i->name.length()))
		   && ((type == Function::Type::NONE) || (type == i->type))) {
			res = i;
		}
	}
	return res;
}

bool ExpressionParser::isWhitespace(char c)
{
	return (settings.whitespaces.find(c) != std::string::npos);
}

bool ExpressionParser::isParenthesis(char c)
{
	return (c == '(') || (c == ')') || (c == '[') || (c == ']');
}

bool ExpressionParser::isVarBeginning(char c)
{
	return isalpha(c) || isdigit(c) || (c == '_');
}

bool ExpressionParser::isOperator(size_t id)
{
	return findItem(id, settings.operators) != settings.operators.end();
}

bool ExpressionParser::isFunction(size_t id)
{
	// Function should be of the following form:
	// TODO: fix errors in regexp:
	// "^[[:alpha:]][[:alnum:]]*[[:space:]]*("
	size_t start = id;
	while((id < str.length()) && (isalpha(str[id]) || (isdigit(str[id])))) ++id;
	if((id < str.length()) && (id > start)) {
		while((id < str.length()) && isWhitespace(str[id])) ++id;
		return (id < str.length()) && (str[id] == '(');
	}
	return false;
}

bool ExpressionParser::isConstant(size_t id)
{
	return isdigit(str[id]) || ((str[id] == '-') && (id + 1 < str.length()) && isdigit(str[id + 1]));
}

int ExpressionParser::seekVar(size_t id)
{
	while((id < str.length()) && (isalpha(str[id]) || isdigit(str[id]) || (str[id] == '_'))) ++id;
	return id;
}

int ExpressionParser::seekNumber(size_t id)
{
	bool found_dot = false;
	while((id < str.length()) && ((isdigit(str[id]) || (str[id] == '.')))) {
		if(str[id] == '.') {
			if(found_dot) {
				throwError("Found second dot in a real number: ", id);
			}
			found_dot = true;
		}
		++id;
	}
	return id;
}

void ExpressionParser::skipWhitespaces()
{
	while((id < str.length()) && isWhitespace(id)) ++id;
}
