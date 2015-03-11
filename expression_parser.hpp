#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <list>
#include <cassert>
#include <exception>
#include <regex>
#include <stack>
#include <sstream>

#include <iostream>

#include "expression_base.hpp"
#include "expression_cell.hpp"

template <typename T>
class ExpressionParser
{
public:
	enum LexemeType {FUNCTION, PARENTHESIS, OPERATOR, ARGUMENT, UNKNOWN};
	struct Lexeme
	{
		Lexeme(LexemeType type_, size_t begin_id_, size_t cur_id_) :
			type(type_), begin_id(begin_id_), cur_id(cur_id_)
		{
		}
		LexemeType type;
		size_t begin_id, cur_id;
	};

	ExpressionParser(ExpressionParserSettings <T> &s, const std::string &_str);
	Cell <T>* parse();
protected:
	void parseNextToken();
	void parseConstant(size_t end_id);
	void parseParenthesisBegin(size_t end_id);
	void parseParenthesisEnd(size_t end_id);
	void parseVariable(size_t end_id);
	void parseOperatorBegin();
	void parseFunctionBegin(size_t id, size_t end_id);
	void parseFunctionArg(size_t id);
	void parseFunctionEnd(size_t id);

	void throwError(const std::string &msg, size_t id) const;

	typename Functions<T>::const_iterator findItem(size_t id, const Functions <T> &coll,
	                                               typename Function<T>::Type type = Function<T>::Type::NONE);

	bool isOperator(size_t id);


	// Returns length of match (zero in case there is no match)
	size_t matchRegex(const std::regex &e);

	ExpressionParserSettings <T> &settings;

	bool is_prev_num;
	// Each function and parenthesis pushes it's own object vector to the stack. This is mainly used for resolvig operators ordering.
	std::stack <std::vector <Cell <T>*> > parents;
	// Top of this stack is always equals current cell of the current environment. Each function and parenthesis
	// creates it's own environment.
	std::stack <Cell <T>*> cells;

	// Each function and parenthesis pushes it's own object of class Lexeme. This is mainly used for displaying errors.
	std::stack <Lexeme> lexems;

	// For displaying errors
	const std::string &str;
};

using std::cout;
using std::endl;

template <typename T>
ExpressionParser<T>::ExpressionParser(ExpressionParserSettings <T> &_settings,
                                      const std::string &_str) :
	settings(_settings), str(_str)
{
}

template <typename T>
Cell <T>* ExpressionParser<T>::parse()
{
	if(str.length() == 0) {
		return nullptr;
	}
	size_t id = 0;
	lexems.push(Lexeme(LexemeType::UNKNOWN, id, id));
	parents.push(std::vector <Cell <T>*>());
	cells.push(new Cell <T>());
	is_prev_num = false;
	while(lexems.top().cur_id < str.length()) {
		parseNextToken();
	}
	if(lexems.top().type == LexemeType::PARENTHESIS) {
		throwError("Mismatched parentheses: ", lexems.top().begin_id);
	}
	if(lexems.top().type == LexemeType::FUNCTION) {
		throwError("Unfinished function call: ", lexems.top().begin_id);
	}
	// if(cells.top()->type == Cell<T>::Type::NONE) {
	// 	throwError("Right argument for operator not found: ", last_op_id);
	// }
	Cell <T> *res = nullptr;
	if(parents.top().empty()) {
		res = cells.top();
	} else {
		res = parents.top()[0];
	}
	// res->sort();
	return res;
}

template <typename T>
void ExpressionParser<T>::parseNextToken()
{
	size_t len = 0;
	if((len = matchRegex(settings.regex_whitespace))) {
		lexems.top().cur_id += len;
	} else if((len = matchRegex(settings.regex_constant))) {
		parseConstant(lexems.top().cur_id + len);
	} else if((len = matchRegex(settings.regex_parenthesis_begin))) {
		parseParenthesisBegin(lexems.top().cur_id + len);
	} else if((lexems.top().type == LexemeType::PARENTHESIS)
	          && (len = matchRegex(settings.regex_parenthesis_end))) {
		parseParenthesisEnd(lexems.top().cur_id + len);
	} else if(isOperator(lexems.top().cur_id)) {
		parseOperatorBegin();
	} else if((len = matchRegex(settings.regex_function_begin))) {
		parseFunctionBegin(lexems.top().cur_id, lexems.top().cur_id + len);
	} else if((lexems.top().type == LexemeType::FUNCTION)
	          && (len = matchRegex(settings.regex_function_end))) {
		parseFunctionEnd(lexems.top().cur_id + len);
	} else if((lexems.top().type == LexemeType::FUNCTION)
	          && (len = matchRegex(settings.regex_func_args_separator))) {
		parseFunctionArg(lexems.top().cur_id + len);
	} else if((len = matchRegex(settings.regex_variable))) {
		parseVariable(lexems.top().cur_id + len);
	} else {
		throwError("Unrecognised token: ", lexems.top().cur_id);
	}
}

template <typename T>
void ExpressionParser<T>::parseVariable(size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", lexems.top().cur_id);
	}
	int start = lexems.top().cur_id;
	std::string varname = str.substr(start, end_id - start);
	cells.top()->type = Cell<T>::Type::VARIABLE;
	cells.top()->var.name = varname;
	is_prev_num = true;

	lexems.top().cur_id = end_id;

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

template <typename T>
void ExpressionParser<T>::parseConstant(size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", lexems.top().cur_id);
	}
	int start = lexems.top().cur_id;
	std::stringstream ss(str.substr(start, end_id - start));
	T val;
	ss >> val;
	cells.top()->type = Cell<T>::Type::CONSTANT;
	cells.top()->val = val;
	is_prev_num = true;
	if(lexems.top().type == LexemeType::OPERATOR) {
		lexems.pop();
	}
	lexems.top().cur_id = end_id;
}

template <typename T>
void ExpressionParser<T>::parseParenthesisBegin(size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", lexems.top().cur_id);
	}
	Cell <T> *cell = cells.top();
	cells.push(cell);
	parents.push(std::vector <Cell <T>*>());
	lexems.push(Lexeme(LexemeType::PARENTHESIS, lexems.top().cur_id, end_id));
}

template <typename T>
void ExpressionParser<T>::parseParenthesisEnd(size_t end_id)
{
	Cell <T> *cell = nullptr;
	if(parents.top().empty()) {
		cell = cells.top();
	} else {
		cell = parents.top()[0];
	}
	cells.pop();
	parents.pop();
	if(!parents.top().empty()) {
		size_t args_num = parents.top()[parents.top().size() - 1]->func.args.size();
		parents.top()[parents.top().size() - 1]->func.args[args_num - 1] = cell;
	}
	cells.top() = cell;

	lexems.pop();
	if(lexems.top().type == LexemeType::OPERATOR) {
		lexems.pop();
	}
	lexems.top().cur_id = end_id;
	is_prev_num = true;
}

template <typename T>
void ExpressionParser<T>::parseOperatorBegin()
{
	typename Functions<T>::const_iterator f;
	size_t id = lexems.top().cur_id;
	Cell <T> *op_cell = new Cell <T>();
	op_cell->type = Cell <T>::Type::FUNCTION;
	if(!is_prev_num) {
		// We have to parse it as prefix operator because previous token is some operator.
		f = findItem(id, settings.operators, Function<T>::Type::PREFIX);
		if(f != settings.operators.end()) {
			Cell <T> *arg_cell = cells.top();
			op_cell->func.args.push_back(arg_cell);
			lexems.push(Lexeme(LexemeType::OPERATOR, id, id + f->name.length()));
		} else {
			throwError("Expected prefix operator: ", id);
		}
	} else {
		// We have to parse it as infix/postfix operator because previous token is some value.
		// First argument for these operators is already stored for us in cells.top(), so we don't have to do anything.
		if((f = findItem(id, settings.operators, Function<T>::Type::INFIX)) != settings.operators.end()) {
			Cell <T> *arg1_cell = cells.top();
			Cell <T> *arg2_cell = new Cell <T>();
			op_cell->func.args.push_back(arg1_cell);
			op_cell->func.args.push_back(arg2_cell);
			cells.top() = arg2_cell;
			is_prev_num = false;
		} else if((f = findItem(id, settings.operators, Function<T>::Type::POSTFIX)) != settings.operators.end()) {
			Cell <T> *arg_cell = cells.top();
			op_cell->func.args.push_back(arg_cell);
			is_prev_num = true;
		} else {
			throwError("Expected postifx or prefix operator ", id);
		}
		// Move id forward
		lexems.top().cur_id += f->name.length();
	}
	op_cell->func.iter = f;
	if(!parents.top().empty()) {
		int id = parents.top().size() - 1;
		Cell <T> *last_par = nullptr;
		while((id >= 0) && (f->precedence < parents.top()[id]->func.iter->precedence)) {
			last_par = parents.top()[id];
			parents.top().pop_back();
			--id;
		}
		if(id >= 0) {
			size_t args_num = parents.top()[id]->func.args.size();
			parents.top()[id]->func.args[args_num - 1] = op_cell;
			if(last_par != nullptr) {
				op_cell->func.args[0] = last_par;
			}
			parents.top().push_back(op_cell);
		} else {
			op_cell->func.args[0] = last_par;
			Cell <T> *cell = op_cell;
			while(cell != cells.top()) {
				parents.top().push_back(cell);
				cell = cell->func.args[cell->func.args.size() - 1];
			}
		}
	} else {
		parents.top().push_back(op_cell);
	}
}

template <typename T>
void ExpressionParser<T>::parseFunctionBegin(size_t id, size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator: ", id);
	}
	auto f = findItem(id, settings.functions);
	if(f == settings.functions.end()) {
		throwError("Undefined function: ", id);
	}
	Cell <T> *cell = cells.top();
	Cell <T> *arg_cell = new Cell <T>();
	cell->type = Cell <T>::Type::FUNCTION;
	cell->func.args.push_back(arg_cell);
	cell->func.iter = f;
	cells.push(arg_cell);
	std::vector <Cell <T>*> tv;
	tv.push_back(cell);
	parents.push(tv);
	lexems.push(Lexeme(LexemeType::FUNCTION, id, end_id));
}

template <typename T>
void ExpressionParser<T>::parseFunctionArg(size_t id)
{
	if(cells.top()->type == Cell <T>::Type::NONE) {
		throwError("Unfinished expression: ", lexems.top().cur_id);
	}
	if(parents.top()[0]->func.iter->args_num == parents.top()[0]->func.args.size()) {
		throwError("Excess argument: ", id);
	}
	Cell <T> *arg_cell = new Cell <T>();
	parents.top()[0]->func.args.push_back(arg_cell);
	cells.top() = arg_cell;
	lexems.top().cur_id = id;
	is_prev_num = false;
}

template <typename T>
void ExpressionParser<T>::parseFunctionEnd(size_t id)
{
	// Set current cell to the funciton cell
	cells.pop();
	cells.top() = parents.top()[0];
	parents.pop();
	lexems.pop();
	lexems.top().cur_id = id;
}

template <typename T>
void ExpressionParser<T>::throwError(const std::string &msg, size_t id) const
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

template <typename T>
typename Functions<T>::const_iterator ExpressionParser<T>::findItem(size_t id, const Functions<T> &coll,
                                                        typename Function<T>::Type type)
{
	typename Functions<T>::const_iterator res = coll.end();
	for(auto i = coll.begin(); i != coll.end(); ++i) {
		if((str.length() - id >= i->name.length())
		   && (str.substr(id, i->name.length()) == i->name)
		   && ((res == coll.end()) || (res->name.length() < i->name.length()))
		   && ((type == Function<T>::Type::NONE) || (type == i->type))) {
			res = i;
		}
	}
	return res;
}

template <typename T>
bool ExpressionParser<T>::isOperator(size_t id)
{
	return findItem(id, settings.operators) != settings.operators.end();
}

template <typename T>
size_t ExpressionParser<T>::matchRegex(const std::regex &e)
{
	std::smatch sm;
	if(regex_search(str.begin() + lexems.top().cur_id, str.end(), sm, e) && (sm.position() == 0)) {
		return sm.length();
	} else {
		return 0;
	}
}

#endif
