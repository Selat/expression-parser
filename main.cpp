#include <iostream>
#include <vector>
#include <functional>

#include "expression.hpp"

using namespace std;

int main()
{
	string s;
	getline(cin, s);
	Expression e1(s);
	getline(cin, s);
	Expression e2(s);
	cout << e1.isSubExpression(e2) << endl;
	return 0;
}
