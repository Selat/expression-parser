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

	std::map <std::string, double>& variables();
	double getVar(size_t id) const;
	double getVar(const std::string &name) const;
	void setVar(size_t id, double val);
	void setVar(const std::string &name, double val);

	double eval();

	void print();
protected:
	Functions::const_iterator findFunction(const std::string &name, Function::Type type);
	void addFunction(const Functions::const_iterator &f, const Expression &e);

	Cell *m_root;
	std::map <std::string, double> m_variables;
	std::vector <std::string> m_varnames;
	static const std::string m_whitespaces;
	static const Functions m_operators;
	static const Functions m_functions;
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
