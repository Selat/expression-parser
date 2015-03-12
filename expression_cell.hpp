#ifndef CELL_H
#define CELL_H

#include "expression_base.hpp"

#include <stack>

#include <iostream>

using std::cout;
using std::endl;

template <typename T>
struct Cell
{
	Cell();
	Cell(const Cell &c);
	~Cell();

	Cell& operator=(const Cell &c);

	bool operator<(const Cell &c) const;
	bool operator==(const Cell &c) const;

	void print() const;
	void printNonRecursive() const;
	void sort();
	T eval(const std::map <std::string, T> &vars);
	bool isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const;

	enum class Type {FUNCTION, CONSTANT, VARIABLE, NONE} type;
	struct
	{
		typename Functions<T>::const_iterator iter;
		std::vector<Cell*> args;
	} func;
	struct
	{
		std::string name;
	} var;
	T val;

	class iterator
	{
	public:
		iterator(const iterator &it) :
			m_parents(it.m_parents),
			m_arg_id(it.m_arg_id)
		{
		}
		iterator& operator=(const iterator &it)
		{
			m_parents = it.m_parents;
			m_arg_id = it.m_arg_id;
			m_curcell = it.m_curcell;
		}
		iterator& operator++()
		{
			if(!m_parents.empty()) {
				if((m_arg_id.top() + 1 < m_parents.top()->func.args.size())) {
					++m_arg_id.top();
					m_curcell = m_parents.top()->func.args[m_arg_id.top()];
					while(m_curcell->type == Cell::Type::FUNCTION) {
						m_parents.push(m_curcell);
						m_arg_id.push(0);
						m_curcell = m_curcell->func.args[0];
					}
				} else {
					m_curcell = m_parents.top();
					m_parents.pop();
					m_arg_id.pop();
				}
			} else {
				m_curcell = nullptr;
			}
			return *this;
		}
		bool operator!=(const iterator &it)
		{
			return m_curcell != it.m_curcell;
		}
		Cell& operator*()
		{
			return *m_curcell;
		}
		Cell* operator->()
		{
			return m_curcell;
		}
	private:
		iterator(Cell *cell)
		{
			m_curcell = cell;
			if(m_curcell != nullptr) {
				while(m_curcell->type == Cell::Type::FUNCTION) {
					m_parents.push(m_curcell);
					m_arg_id.push(0);
					m_curcell = m_curcell->func.args[0];
				}
			}
		}
		friend Cell;

		std::stack <Cell <T>*> m_parents;
		std::stack <int> m_arg_id;
		Cell *m_curcell;
	};

	iterator begin()
	{
		return iterator(this);
	}
	iterator end()
	{
		return iterator(nullptr);
	}
};


template <typename T>
Cell<T>::Cell() :
	type(Type::NONE)
{
}

template <typename T>
Cell<T>::Cell(const Cell &c)
{
	*this = c;
}

template <typename T>
Cell<T>::~Cell()
{
	if(type == Type::FUNCTION) {
		for(auto i : func.args) {
			delete i;
		}
	}
}

template <typename T>
Cell<T>& Cell<T>::operator=(const Cell &c)
{
	type = c.type;
	switch(type) {
	case Type::FUNCTION:
	{
		func.iter = c.func.iter;
		for(auto i : c.func.args) {
			func.args.push_back(new Cell(*i));
		}
		break;
	}
	case Type::VARIABLE:
	{
		var.name = c.var.name;
		break;
	}
	case Type::CONSTANT:
	{
		val = c.val;
		break;
	}
	default:
	{
		break;
	}
	}
	return *this;
}

template <typename T>
bool Cell<T>::operator<(const Cell &c) const
{
	if((c.type == Type::FUNCTION) && (type == Type::FUNCTION)) {
		auto i1 = func.iter, i2 = c.func.iter;
		return (i1->name < i2->name) || ((i1->name == i2->name) && (i1->args_num < i2->args_num));
	} else if((c.type == Type::VARIABLE) && (type == Type::VARIABLE)) {
		return var.name < c.var.name;
	} else if((c.type == Type::CONSTANT) && (type == Type::CONSTANT)) {
		return val < c.val;
	} else if(((type == Type::VARIABLE) && (c.type == Type::CONSTANT))
	          || (type == Type::FUNCTION)) {
		return true;
	} else {
		return false;
	}
}

template <typename T>
bool Cell<T>::operator==(const Cell &c) const
{
	if((type == Type::FUNCTION) && (c.type == Type::FUNCTION) && (func.iter == c.func.iter)) {
		bool ok = true;
		for(size_t i = 0; i < func.args.size(); ++i) {
			if(!(*func.args[i] == *c.func.args[i])) {
				ok = false;
			}
		}
		return ok;
	} else if((type == Type::VARIABLE) && (c.type == Type::VARIABLE)) {
		return var.name == c.var.name;
	} else if((type == Type::CONSTANT) && (c.type == Type::CONSTANT)) {
		return val == c.val;
	} else {
		return false;
	}
}

template <typename T>
void Cell<T>::print() const
{
	if(type == Type::FUNCTION) {
		cout << "(";
		cout << func.iter->name;
		for(const auto &i : func.args) {
			cout << " ";
			i->print();
		}
		cout << ")";
	} else if(type == Type::VARIABLE) {
		cout << var.name;
	} else if(type == Type::CONSTANT) {
		cout << val;
	}
}

template <typename T>
void Cell<T>::printNonRecursive() const
{
	if(type == Type::FUNCTION) {
		cout << "func: ";
		cout << func.iter->name;
	} else if(type == Type::VARIABLE) {
		cout << "var: " << var.name;
	} else if(type == Type::CONSTANT) {
		cout << "const: " << val;
	}
}

template <typename T>
T Cell<T>::eval(const std::map <std::string, T> &vars)
{
	switch(type) {
	case Type::FUNCTION:
	{
		Args <T> args;
		for(auto i : func.args) {
			args.push_back(i->eval(vars));
		}
		return func.iter->func(args);
	}
	case Type::VARIABLE:
	{
		auto it = vars.find(var.name);
		return it->second;
	}
	case Type::CONSTANT:
	{
		return val;
	}
	default:
		throw ExpressionParserException("Attempt to evaluate cell of type \"NONE\"");
	}
}

template <typename T>
bool Cell<T>::isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const
{
	// Invariant relation:
	// curcell - path from root to one of the leaves in a assumed subtree that
	// hasn't been matched yet
	if(type == Type::FUNCTION) {
		bool tsm;
		subtree_match = true;
		for(size_t i = 0; i < func.iter->args_num; ++i) {
			if(func.args[i]->isSubExpression(curcell, tsm)) {
				return true;
			}
			subtree_match &= tsm;
			// It's possible that recursive call has changed value of curcell, so we have
			// to restore it (i.e. push necessary arguments)
			auto cell = curcell[curcell.size() - 1];
			if((cell->type == Type::FUNCTION) && (i + 1 < func.iter->args_num)) {
				if(func.iter != cell->func.iter) {
					subtree_match = false;
				} else {
					curcell.push_back(cell->func.args[i + 1]);
					while(curcell[curcell.size() - 1]->type == Cell<T>::Type::FUNCTION) {
						curcell.push_back(curcell[curcell.size() - 1]->func.args[0]);
					}

				}
			}
		}
		auto cell = curcell[curcell.size() - 1];
		if(subtree_match && (func.iter == cell->func.iter)) {
			curcell.pop_back();
			return (curcell.size() == 0);
		} else {
			subtree_match = false;
			while(cell->type == Cell<T>::Type::FUNCTION) {
				curcell.push_back(cell = cell->func.args[0]);
			}
			return false;
		}
	} else if(curcell.size() > 1) {
		subtree_match = (*this == *curcell[curcell.size() - 1]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return false;
	} else {
		subtree_match = (*this == *curcell[0]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return subtree_match;
	}
}

template <typename T>
void Cell<T>::sort()
{
	if(type == Type::FUNCTION) {
		auto f = func.iter;
		if((f->args_num == 2) && f->is_commutative && (*func.args[1] < *func.args[0])) {
			std::swap(func.args[0], func.args[1]);
		}
		for(auto i : func.args) {
			i->sort();
		}
	}
}

#endif
