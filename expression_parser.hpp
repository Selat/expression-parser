#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <list>
#include <cassert>
#include <exception>

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
		whitespaces(_whitespaces), operators(_operators), functions(_functions), variables(_variables)
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
};

class ExpressionParser
{
public:
	ExpressionParser(ExpressionParserSettings &s, const std::string &_str);
	Cell* parse();
	Cell* _parse(size_t &id);
	void parseNextToken();
	void parseNumber();
	void parseVariable();
	void parseParenthesis();
	void parseOperator();
	void parseFunction();

	void throwError(const std::string &msg, size_t id);

	Functions::const_iterator findItem(size_t id, const Functions &coll,
									   Function::Type type = Function::Type::NONE);
	bool isWhitespace(char c);
	bool isParenthesis(char c);
	bool isVarBeginning(char c);

	bool isOperator(size_t id);
	bool isFunction(size_t id);

	bool isConstant(size_t id);

	int seekNumber(size_t id);
	int seekVar(size_t id);
	void skipWhitespaces();
protected:
	ExpressionParserSettings &settings;

	size_t id;
	Cell *root;
	Cell *curcell;
	bool is_prev_num;
	std::vector <Cell*> parents;

	// For displaying errors
	const std::string &str;
	int last_op_id;
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
