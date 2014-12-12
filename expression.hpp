#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <list>
#include <cassert>

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
	const Function &operator=(const Function &f)
	{
		return *this;
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
	Cell() :
		type(Type::NONE)
	{
	}
	~Cell()
	{
	}

	void print();
	double eval();

	enum Type {FUNCTION, NUMBER, VARIABLE, NONE} type;
	struct
	{
		Functions::const_iterator iter;
		std::vector<Cell*> args;
	} func;
	struct
	{
		std::map <std::string, double>::iterator iter;
	} var;
	double val;
};

struct ExpressionParserSettings
{
public:
	ExpressionParserSettings(const std::string &_whitespaces, const Functions &_operators, const Functions &_functions,
	                         std::map <std::string, double> &_variables) :
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
	std::map <std::string, double> &variables;
};

class ExpressionParser
{
public:
	ExpressionParser(ExpressionParserSettings &s);
	Cell* parse(const std::string &s);
	void parseNextToken(const std::string &s);
	void parseNumber(const std::string &s);
	void parseVariable(const std::string &s);
	void parseParenthesis(const std::string &s);
	void parseOperator(const std::string &s);
	void parseFunction(const std::string &s);

	void throwError(const std::string &msg, size_t id);

	size_t findMatchingParenthesis(const std::string &s, size_t id);
	static Functions::const_iterator findItem(const std::string &s, size_t id, const Functions &coll,
	                                          Function::Type type = Function::Type::NONE);
	bool isWhitespace(char c);
	bool isParenthesis(char c);
	bool isVarBeginning(char c);

	bool isOperator(const std::string &s, size_t id);
	bool isFunction(const std::string &s, size_t id);

	bool isConstant(const std::string &s, size_t id);

	int seekNumber(const std::string &s, size_t id);
	int seekVar(const std::string &s, size_t id);
protected:
	ExpressionParser(ExpressionParserSettings &s,
	                 const std::string &_real_s, size_t shift);

	ExpressionParserSettings &settings;

	size_t id;
	Cell *root;
	Cell *curcell;
	bool is_prev_num;
	std::vector <Cell*> parents;

	// For displaying errors
	std::string real_s;
	size_t real_shift;
	bool is_recursive_call;
	int last_op_id;
};

class ExpressionException : public std::exception
{
public:
	ExpressionException(const std::string &s) throw() : m_s(s) {}
	virtual const char* what() const throw()
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
};

class Expression
{
public:
	Expression(const std::string &s);
protected:
	Cell *m_root;
	std::map <std::string, double> m_variables;
	static const std::string m_whitespaces;
	static const Functions m_operators;
	static const Functions m_functions;
};

#endif
