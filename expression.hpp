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
typedef const std::vector <double> Args;

struct Function
{
	enum Type {PREFIX, INFIX, POSTFIX};
	// Precedence is only for operators
	Function(const std::string &s, int p, const FuncLambda &f, Type _type = Type::INFIX, size_t _args_num = 2) :
		name(s), precedence(p), func(f), type(_type), args_num(_args_num)
	{
		if(type == Type::INFIX) {
			assert(_args_num == 2);
			args_num = 2;
		} else {
			args_num = _args_num;
		}
	}
	Function(const std::string &s, const FuncLambda &f, int n = 1) :
		name(s), precedence(0), func(f), args_num(n)
	{
	}
	std::string name;
	int precedence;
	const FuncLambda &func;
	Type type;
	size_t args_num;
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

class ExpressionParser
{
public:
	ExpressionParser(std::map <std::string, double> &variables);
	Cell* parse(const std::string &s);
	void parseNumber(const std::string &s);
	void parseVariable(const std::string &s);
	void parseParenthesis(const std::string &s);
	void parseOperator(const std::string &s);
	void parseFunction(const std::string &s);

	void throwError(const std::string &msg, size_t id);

	size_t findMatchingParenthesis(const std::string &s, size_t id);
	static Functions::const_iterator findItem(const std::string &s, size_t id, const Functions &coll);
	static bool isWhitespace(char c);
	static bool isParenthesis(char c);
	static bool isVarBeginning(char c);

	static bool isOperator(const std::string &s, size_t id);
	static bool isFunction(const std::string &s, size_t id);

	static bool isConstant(const std::string &s, size_t id);

	int seekNumber(const std::string &s, size_t id);
	int seekVar(const std::string &s, size_t id);
protected:
	ExpressionParser(std::map <std::string, double> &variables,
	                 const std::string &_real_s, size_t shift);

	static const std::string m_whitespaces;
	static const Functions m_operators;
	static const Functions m_functions;

	std::map <std::string, double> &m_variables;

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
};

#endif
