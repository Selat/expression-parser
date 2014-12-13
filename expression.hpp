#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <map>

#include "expression_parser.hpp"

class Expression
{
public:
	Expression(const std::string &s);
	bool operator==(const Expression &e) const;
	bool isSubExpression(const Expression &e) const;
protected:
	Cell *m_root;
	std::map <std::string, double> m_variables;
	static const std::string m_whitespaces;
	static const Functions m_operators;
	static const Functions m_functions;
};

#endif
