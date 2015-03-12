#ifndef EXPRESSION_BASE_H
#define EXPRESSION_BASE_H

#include <functional>
#include <vector>
#include <string>
#include <cassert>

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
	enum class Type {PREFIX, INFIX, POSTFIX, NONE};
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
struct ExpressionParserSettings
{
public:
	ExpressionParserSettings(const Functions<T> &_operators, const Functions <T> &_functions,
	                         std::vector <std::string> &_variables) :
		operators(_operators), functions(_functions), variables(_variables),
		regex_whitespace("^[[:space:]]+"), regex_constant("^[[:digit:]]+"), regex_parenthesis_begin("^\\("),
		regex_parenthesis_end("^\\)"), regex_variable("^[[:alpha:]][[:alnum:]]*"),
		regex_function_begin("^[[:alpha:]][[:alnum:]]*[[:space:]]*\\("), regex_function_end("^\\)"),
		regex_func_args_separator("^,")
	{
	}
	ExpressionParserSettings(const ExpressionParserSettings &s) :
		operators(s.operators), functions(s.functions), variables(s.variables)
	{
	}
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


class ExpressionParserException : public std::exception
{
public:
	ExpressionParserException(const std::string &s) noexcept : m_s(s) {}
	virtual const char* what() const noexcept override
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
};

#endif
