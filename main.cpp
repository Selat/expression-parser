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
	(e2 + e1).print(); cout << endl;
	(e2 - e1).print(); cout << endl;
	(e2 * e1).print(); cout << endl;
	(e2 / e1).print(); cout << endl;
	e2.print();
	int n;
	cin >> n;
	for(int i=  0; i < n; ++i) {
		size_t id;
		cin >> id;
		cout << e1.getVar(id) << endl;
	}
	e1.variables()["x"] = 1.0;
	cout << e1.getVar(0) << endl;
	return 0;
}
