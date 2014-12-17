#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <map>

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

	void print()
	{
		m_root->print();
	}
protected:
	Functions::const_iterator findFunction(const std::string &name, Function::Type type);
	void addFunction(const Functions::const_iterator &f, const Expression &e);

	Cell *m_root;
	std::map <std::string, double> m_variables;
	static const std::string m_whitespaces;
	static const Functions m_operators;
	static const Functions m_functions;
};

#endif
