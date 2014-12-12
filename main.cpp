#include <iostream>
#include <vector>
#include <functional>

#include "expression.hpp"

using namespace std;

int main()
{
	string s;
	getline(cin, s);
	Expression e(s);
	return 0;
}
