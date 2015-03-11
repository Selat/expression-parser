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

#include <iostream>

struct Function;

typedef std::function<double(const std::vector <double>&)> FuncLambda;
typedef std::vector<Function> Functions;
typedef std::vector <double> Args;
typedef std::map <std::string, double> Vars;

struct Function
{
	enum Type {PREFIX, INFIX, POSTFIX, NONE};
	// Precedence is only for operators
	// For prefix/postfix operators (these always have exactly one argument).
	Function(const std::string &s, int p, const FuncLambda &f, Type _type) :
		name(s), precedence(p), func(f), type(_type), args_num(1), is_commutative(false)
	{
		assert(type != Type::INFIX);
	}

	// For infix operators
	Function(const std::string &s, int p, const FuncLambda &f, bool _is_commutative) :
		name(s), precedence(p), func(f), type(Type::INFIX), args_num(2), is_commutative(_is_commutative)
	{
	}

	// For functions
	Function(const std::string &s, const FuncLambda &f, int n = 1) :
		name(s), precedence(0), func(f), args_num(n), is_commutative(false)
	{
	}

	Function(const Function &f) :
		name(f.name), precedence(f.precedence), func(f.func), type(f.type), args_num(f.args_num), is_commutative(f.is_commutative)
	{
	}
	std::string name;
	int precedence;
	const FuncLambda func;
	Type type;
	size_t args_num;
	bool is_commutative;
};

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
	double eval(const std::map <std::string, double> &vars);
	bool isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const;

	enum Type {FUNCTION, NUMBER, VARIABLE, NONE} type;
	struct
	{
		Functions::const_iterator iter;
		std::vector<Cell*> args;
	} func;
	struct
	{
		std::string name;
	} var;
	double val;
};

struct ExpressionParserSettings
{
public:
	ExpressionParserSettings(const std::string &_whitespaces, const Functions &_operators, const Functions &_functions,
	                         std::vector <std::string> &_variables) :
		whitespaces(_whitespaces), operators(_operators), functions(_functions), variables(_variables),
		regex_whitespace("^[[:space:]]+"), regex_constant("^[[:digit:]]+"), regex_parenthesis_begin("^\\("),
		regex_parenthesis_end("^\\)"), regex_function_begin("^[[:alpha:]][[:alnum:]]*[[:space:]]*\\("), regex_function_end("^\\)"),
		regex_func_args_separator("^,")
	{
	}
	ExpressionParserSettings(const ExpressionParserSettings &s) :
		whitespaces(s.whitespaces), operators(s.operators), functions(s.functions), variables(s.variables)
	{
	}
	const std::string &whitespaces;
	const Functions &operators;
	const Functions &functions;
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

	ExpressionParser(ExpressionParserSettings &s, const std::string &_str);
	Cell* parse();
	Cell* _parse(size_t id);
	void parseNextToken();
	void parseConstant(size_t end_id);
	void parseParenthesisBegin(size_t end_id);
	void parseParenthesisEnd(size_t end_id);
	void parseVariable();
	void parseOperatorBegin();
	void parseFunctionBegin(size_t id, size_t end_id);
	void parseFunctionArg(size_t id);
	void parseFunctionEnd(size_t id);

	void throwError(const std::string &msg, size_t id) const;

	Functions::const_iterator findItem(size_t id, const Functions &coll,
									   Function::Type type = Function::Type::NONE);
	bool isVarBeginning(char c);

	bool isOperator(size_t id);
	bool isFunction(size_t id);
protected:
	// Returns length of match (zero in case there is no match)
	size_t matchRegex(const std::regex &e);

	ExpressionParserSettings &settings;

	bool is_prev_num;
	std::stack <std::vector <Cell*> > parents;
	std::stack <Cell*> cells;

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

#endif
