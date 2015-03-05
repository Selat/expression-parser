#include <iostream>
#include <vector>
#include <functional>

#include "expression.hpp"

using namespace std;

void testSpeed()
{
	string s;
	int n = 50000;
	for(int i=  0; i < n; ++i) {
		s += "(";
	}
	s += "x";
	for(int i=  0; i < n; ++i) {
		s += ")";
	}
	Expression e1(s);
}

int main()
{
	try {
//		testSpeed();
		string s;
		getline(cin, s);
		Expression e1(s);
	} catch(std::exception &e) {
		cout << e.what() << endl;
	}
	return 0;
}
