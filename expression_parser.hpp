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

template <typename T>
struct Function;

template <typename T>
using FuncLambda = std::function<T(const std::vector <T>&)>;
template <typename T>
using Functions = std::vector<Function<T> >;
template <typename T>
using Args = std::vector <T>;
template <typename T>
using Vars = std::map <std::string, T>;

template <typename T>
struct Function
{
	enum Type {PREFIX, INFIX, POSTFIX, NONE};
	// Precedence is only for operators
	// For prefix/postfix operators (these always have exactly one argument).
	Function(const std::string &s, int p, const FuncLambda <T> &f, Type _type) :
		name(s), precedence(p), func(f), type(_type), args_num(1), is_commutative(false)
	{
		assert(type != Type::INFIX);
	}

	// For infix operators
	Function(const std::string &s, int p, const FuncLambda <T> &f, bool _is_commutative) :
		name(s), precedence(p), func(f), type(Type::INFIX), args_num(2), is_commutative(_is_commutative)
	{
	}

	// For functions
	Function(const std::string &s, const FuncLambda <T> &f, int n = 1) :
		name(s), precedence(0), func(f), args_num(n), is_commutative(false)
	{
	}

	Function(const Function <T> &f) :
		name(f.name), precedence(f.precedence), func(f.func), type(f.type), args_num(f.args_num), is_commutative(f.is_commutative)
	{
	}
	std::string name;
	int precedence;
	const FuncLambda <T> func;
	Type type;
	size_t args_num;
	bool is_commutative;
};

template <typename T>
struct Cell
{
	Cell();
	Cell(const Cell &c);
	~Cell();

	Cell& operator=(const Cell &c);

	bool operator<(const Cell &c) const;
	bool operator==(const Cell &c) const;

	void print() const;
	void sort();
	T eval(const std::map <std::string, T> &vars);
	bool isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const;

	enum Type {FUNCTION, CONSTANT, VARIABLE, NONE} type;
	struct
	{
		typename Functions<T>::const_iterator iter;
		std::vector<Cell*> args;
	} func;
	struct
	{
		std::string name;
	} var;
	T val;
};

template <typename T>
struct ExpressionParserSettings
{
public:
	ExpressionParserSettings(const std::string &_whitespaces, const Functions<T> &_operators, const Functions <T> &_functions,
	                         std::vector <std::string> &_variables) :
		whitespaces(_whitespaces), operators(_operators), functions(_functions), variables(_variables),
		regex_whitespace("^[[:space:]]+"), regex_constant("^[[:digit:]]+"), regex_parenthesis_begin("^\\("),
		regex_parenthesis_end("^\\)"), regex_variable("^[[:alpha:]][[:alnum:]]*"),
		regex_function_begin("^[[:alpha:]][[:alnum:]]*[[:space:]]*\\("), regex_function_end("^\\)"),
		regex_func_args_separator("^,")
	{
	}
	ExpressionParserSettings(const ExpressionParserSettings &s) :
		whitespaces(s.whitespaces), operators(s.operators), functions(s.functions), variables(s.variables)
	{
	}
	const std::string &whitespaces;
	const Functions <T> &operators;
	const Functions <T> &functions;
	std::vector <std::string> &variables;

	std::regex regex_whitespace;
	std::regex regex_constant;
	std::regex regex_parenthesis_begin;
	std::regex regex_parenthesis_end;
	std::regex regex_variable;
	std::regex regex_function_begin;
	std::regex regex_function_end;
	std::regex regex_func_args_separator;
};

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
	Cell <T>* _parse(size_t id);
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
protected:
	// Returns length of match (zero in case there is no match)
	size_t matchRegex(const std::regex &e);

	ExpressionParserSettings <T> &settings;

	bool is_prev_num;
	std::stack <std::vector <Cell <T>*> > parents;
	std::stack <Cell <T>*> cells;

	// First element - id from which parsing started, second - current id.
	std::stack <Lexeme> lexems;

	// For displaying errors
	const std::string &str;
};

class ExpressionParserException : public std::exception
{
public:
	ExpressionParserException(const std::string &s) throw() : m_s(s) {}
	virtual const char* what() const throw()
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
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
	Cell <T> *res = _parse(id);
//	res->sort();
	return res;
}

template <typename T>
Cell <T>* ExpressionParser<T>::_parse(size_t tid)
{
	lexems.push(Lexeme(LexemeType::UNKNOWN, tid, tid));
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
	if(parents.top().empty()) {
		return cells.top();
	} else {
		return parents.top()[0];
	}
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

	// Move id forward
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
		// First argument for these operators is already stored for us in curcell, so we don't have to do anything.
		// Check next token ("value" - for infix operator, "operator" - for postfix operator) to choose right operator.
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

template <typename T>
Cell<T>::Cell() :
	type(Type::NONE)
{
}

template <typename T>
Cell<T>::Cell(const Cell &c)
{
	*this = c;
}

template <typename T>
Cell<T>::~Cell()
{
	if(type == Type::FUNCTION) {
		for(auto i : func.args) {
			delete i;
		}
	}
}

template <typename T>
Cell<T>& Cell<T>::operator=(const Cell &c)
{
	type = c.type;
	switch(type) {
	case Type::FUNCTION:
	{
		func.iter = c.func.iter;
		for(auto i : c.func.args) {
			func.args.push_back(new Cell(*i));
		}
		break;
	}
	case Type::VARIABLE:
	{
		var.name = c.var.name;
		break;
	}
	case Type::CONSTANT:
	{
		val = c.val;
		break;
	}
	default:
	{
		break;
	}
	}
	return *this;
}

template <typename T>
bool Cell<T>::operator<(const Cell &c) const
{
	if((c.type == Type::FUNCTION) && (type == Type::FUNCTION)) {
		auto i1 = func.iter, i2 = c.func.iter;
		return (i1->name < i2->name) || ((i1->name == i2->name) && (i1->args_num < i2->args_num));
	} else if((c.type == Type::VARIABLE) && (type == Type::VARIABLE)) {
		return var.name < c.var.name;
	} else if((c.type == Type::CONSTANT) && (type == Type::CONSTANT)) {
		return val < c.val;
	} else if(((type == Type::VARIABLE) && (c.type == Type::CONSTANT))
	          || (type == Type::FUNCTION)) {
		return true;
	} else {
		return false;
	}
}

template <typename T>
bool Cell<T>::operator==(const Cell &c) const
{
	if((type == Type::FUNCTION) && (c.type == Type::FUNCTION) && (func.iter == c.func.iter)) {
		bool ok = true;
		for(size_t i = 0; i < func.args.size(); ++i) {
			if(!(*func.args[i] == *c.func.args[i])) {
				ok = false;
			}
		}
		return ok;
	} else if((type == Type::VARIABLE) && (c.type == Type::VARIABLE)) {
		return var.name == c.var.name;
	} else if((type == Type::CONSTANT) && (c.type == Type::CONSTANT)) {
		return val == c.val;
	} else {
		return false;
	}
}

template <typename T>
void Cell<T>::print() const
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
		cout << var.name;
	} else if(type == Type::CONSTANT) {
		cout << val;
	}
}

template <typename T>
T Cell<T>::eval(const std::map <std::string, T> &vars)
{
	switch(type) {
	case Type::FUNCTION:
	{
		Args <T> args;
		for(auto i : func.args) {
			args.push_back(i->eval(vars));
		}
		return func.iter->func(args);
	}
	case Type::VARIABLE:
	{
		auto it = vars.find(var.name);
		return it->second;
	}
	case Type::CONSTANT:
	{
		return val;
	}
	default:
		throw ExpressionParserException("Attempt to evaluate cell of type \"NONE\"");
	}
}

template <typename T>
bool Cell<T>::isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const
{
	// Invariant relation:
	// curcell - path from root to one of the leaves in a assumed subtree that
	// hasn't been matched yet
	if(type == Type::FUNCTION) {
		bool tsm;
		subtree_match = true;
		for(size_t i = 0; i < func.iter->args_num; ++i) {
			if(func.args[i]->isSubExpression(curcell, tsm)) {
				return true;
			}
			subtree_match &= tsm;
			// It's possible that recursive call has changed value of curcell, so we have
			// to restore it (i.e. push necessary arguments)
			auto cell = curcell[curcell.size() - 1];
			if((cell->type == Type::FUNCTION) && (i + 1 < func.iter->args_num)) {
				if(func.iter != cell->func.iter) {
					subtree_match = false;
				} else {
					curcell.push_back(cell->func.args[i + 1]);
					while(curcell[curcell.size() - 1]->type == Cell<T>::Type::FUNCTION) {
						curcell.push_back(curcell[curcell.size() - 1]->func.args[0]);
					}

				}
			}
		}
		auto cell = curcell[curcell.size() - 1];
		if(subtree_match && (func.iter == cell->func.iter)) {
			curcell.pop_back();
			return (curcell.size() == 0);
		} else {
			subtree_match = false;
			while(cell->type == Cell<T>::Type::FUNCTION) {
				curcell.push_back(cell = cell->func.args[0]);
			}
			return false;
		}
	} else if(curcell.size() > 1) {
		subtree_match = (*this == *curcell[curcell.size() - 1]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return false;
	} else {
		subtree_match = (*this == *curcell[0]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return subtree_match;
	}
}

template <typename T>
void Cell<T>::sort()
{
	if(type == Type::FUNCTION) {
		auto f = func.iter;
		if((f->args_num == 2) && f->is_commutative && (*func.args[1] < *func.args[0])) {
			std::swap(func.args[0], func.args[1]);
		}
		for(auto i : func.args) {
			i->sort();
		}
	}
}

#endif
