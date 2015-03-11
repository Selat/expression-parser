#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <map>
#include <vector>

#include "expression_parser.hpp"

class Expression
{
public:
	Expression(const std::string &s);
	Expression(const Expression &e);

	Expression& operator=(const Expression &e);

	bool operator==(const Expression &e) const;
	bool operator!=(const Expression &e) const;

	Expression& operator+=(const Expression &e);
	Expression& operator-=(const Expression &e);
	Expression& operator*=(const Expression &e);
	Expression& operator/=(const Expression &e);

	Expression operator+(const Expression &e) const;
	Expression operator-(const Expression &e) const;
	Expression operator*(const Expression &e) const;
	Expression operator/(const Expression &e) const;

	bool isSubExpression(const Expression &e) const;

	std::map <std::string, int>& variables();
	int getVar(size_t id) const;
	int getVar(const std::string &name) const;
	void setVar(size_t id, int val);
	void setVar(const std::string &name, int val);

	int eval();

	void print();
protected:
	Functions<int>::const_iterator findFunction(const std::string &name, Function<int>::Type type);
	void addFunction(const Functions<int>::const_iterator &f, const Expression &e);

	Cell<int> *m_root;
	std::map <std::string, int> m_variables;
	std::vector <std::string> m_varnames;
	static const std::string m_whitespaces;
	static const Functions<int> m_operators;
	static const Functions<int> m_functions;
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

#endif
