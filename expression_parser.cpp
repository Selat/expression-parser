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
//	res->sort();
	return res;
}

Cell* ExpressionParser::_parse(size_t tid)
{
	lexems.push(Lexeme(LexemeType::UNKNOWN, tid, tid));
	parents.push(std::vector <Cell*>());
	cells.push(new Cell());
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
	// if(curcell->type == Cell::Type::NONE) {
	// 	throwError("Right argument for operator not found: ", last_op_id);
	// }
	if(parents.top().empty()) {
		return cells.top();
	} else {
		return parents.top()[0];
	}
}

void ExpressionParser::parseNextToken()
{
	size_t len = 0;
	bool cur_operator = false;
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
		cur_operator = true;
	// } else if((len = matchRegex(settings.regex_func_args_separator))) {
	// 	size_t id = lexems.top().cur_id + len;
	// 	lexems.pop();
	// 	lexems.top().cur_id = id;
	// } else if((len = matchRegex(settings.regex_function_end))) {
	// 	size_t id = lexems.top().cur_id + len;
	// 	lexems.pop();
	// 	lexems.top().cur_id = id;
		// } else if(isOperator(id)) {
		// 	last_op_id = id;
		// 	parseOperator();
		// } else if(isFunction(id)) {
		// 	parseFunction();
		// } else if(isVarBeginning(str[id])) {
		// 	parseVariable();
	} else {
		throwError("Unrecognised token: ", lexems.top().cur_id);
	}
	// if((lexems.top().type == LexemeType::OPERATOR) && !cur_operator) {
	// 	parseOperatorEnd();
	// }
}

void ExpressionParser::parseVariable()
{
	// if(is_prev_num) {
	// 	throwError("Expected operator between two values: ", id);
	// }
	// int start = id;
	// id = seekVar(id);
	// std::string varname = str.substr(start, id - start);
	// curcell->type = Cell::Type::VARIABLE;
	// curcell->var.name = varname;
	// is_prev_num = true;

	// bool exist = false;
	// for(const auto &i : settings.variables) {
	// 	if(i == varname) {
	// 		exist = true;
	// 		break;
	// 	}
	// }
	// if(!exist) {
	// 	settings.variables.push_back(varname);
	// }
}

void ExpressionParser::parseConstant(size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", lexems.top().cur_id);
	}
	int start = lexems.top().cur_id;
	std::stringstream ss(str.substr(start, end_id - start));
	double val;
	ss >> val;
	cells.top()->type = Cell::Type::NUMBER;
	cells.top()->val = val;
	is_prev_num = true;
	if(lexems.top().type == LexemeType::OPERATOR) {
		lexems.pop();
	}
	lexems.top().cur_id = end_id;
}

void ExpressionParser::parseParenthesisBegin(size_t end_id)
{
	if(is_prev_num) {
		throwError("Expected operator between two values: ", lexems.top().cur_id);
	}
	Cell *cell = cells.top();
	cells.push(cell);
	parents.push(std::vector <Cell*>());
	lexems.push(Lexeme(LexemeType::PARENTHESIS, lexems.top().cur_id, end_id));
}

void ExpressionParser::parseParenthesisEnd(size_t end_id)
{
	Cell *cell = nullptr;
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

	// Move id forward
	lexems.pop();
	if(lexems.top().type == LexemeType::OPERATOR) {
		lexems.pop();
	}
	lexems.top().cur_id = end_id;
	is_prev_num = true;
}

void ExpressionParser::parseOperatorBegin()
{
	Functions::const_iterator f;
	size_t id = lexems.top().cur_id;
	Cell *op_cell = new Cell();
	op_cell->type = Cell::Type::FUNCTION;
	if(!is_prev_num) {
		// We have to parse it as prefix operator because previous token is some operator.
		f = findItem(id, settings.operators, Function::Type::PREFIX);
		if(f != settings.operators.end()) {
			Cell *arg_cell = cells.top();
			op_cell->func.args.push_back(arg_cell);
			lexems.push(Lexeme(LexemeType::OPERATOR, id, id + f->name.length()));
		} else {
			throwError("Expected prefix operator: ", id);
		}
	} else {
		// We have to parse it as infix/postfix operator because previous token is some value.
		// First argument for these operators is already stored for us in curcell, so we don't have to do anything.
		// Check next token ("value" - for infix operator, "operator" - for postfix operator) to choose right operator.
		if((f = findItem(id, settings.operators, Function::Type::INFIX)) != settings.operators.end()) {
			Cell *arg1_cell = cells.top();
			Cell *arg2_cell = new Cell();
			op_cell->func.args.push_back(arg1_cell);
			op_cell->func.args.push_back(arg2_cell);
			cells.top() = arg2_cell;
			is_prev_num = false;
		} else if((f = findItem(id, settings.operators, Function::Type::POSTFIX)) != settings.operators.end()) {
			Cell *arg_cell = cells.top();
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
		Cell *last_par = nullptr;
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
			Cell *cell = op_cell;
			while(cell != cells.top()) {
				parents.top().push_back(cell);
				cell = cell->func.args[cell->func.args.size() - 1];
			}
		}
	} else {
		parents.top().push_back(op_cell);
	}
}

void ExpressionParser::parseFunction()
{
	// auto f = findItem(id, settings.functions);
	// if(f == settings.functions.end()) {
	// 	throwError("Undefined function: ", id);
	// }
	// id += f->name.length();
	// size_t func_name_id = id;
	// //skipWhitespaces();

	// if((id >= str.length()) || (str[id] != '(')) {
	// 	throwError("Expected list of parameters after the name of the function: ", id);
	// }
	// curcell->type = Cell::Type::FUNCTION;
	// curcell->func.iter = f;
	// size_t prev_id = id + 1;
	// while(prev_id < str.length()) {
	// 	ExpressionParser p(settings, str);
	// 	curcell->func.args.push_back(p._parse(prev_id));
	// 	if(str[prev_id] == ')') {
	// 		++prev_id;
	// 		break;
	// 	}
	// 	++prev_id;
	// }
	// if(curcell->func.args.size() != f->args_num) {
	// 	throwError("Invalid number of arguments: ", func_name_id);
	// }
	// id = prev_id;
	// is_prev_num = true;
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
//		while((id < str.length()) && isWhitespace(str[id])) ++id;
		return (id < str.length()) && (str[id] == '(');
	}
	return false;
}

size_t ExpressionParser::matchRegex(const std::regex &e)
{
	std::smatch sm;
	if(regex_search(str.begin() + lexems.top().cur_id, str.end(), sm, e) && (sm.position() == 0)) {
		return sm.length();
	} else {
		return 0;
	}
}
