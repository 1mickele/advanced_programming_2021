#pragma once

#include <vector>
#include <iostream>
#include <exception>

template <class T, class N> 
class _iterator;

template <
	class T, 
	typename N = std::size_t 
>
class stack_pool
{
	struct node_t 
	{
		// non funziona con ::construct {v, x} e 
		// node_t{v, x} chiama 2 volte T(T&&)
		template<class Ty>
		node_t(Ty&& v, N x)
			: value{std::forward<Ty>(v)}, next{x}
		{ }

		// se T è primitivo, nessuna inizializzazione,
		// se T è una classe, value è default-constructed
		node_t(N x)
			: next{x}
		{ }

		T value;	
		N next;
	};

	using size_type = std::size_t;

	using value_type = T;
	using stack_type = N;

	std::vector<node_t> pool;

	stack_type free_nodes;

	const static constexpr stack_type _end = stack_type{0};
private:

	node_t& node(stack_type x) noexcept { return pool[x-1]; }
	const node_t& node(stack_type x) const noexcept { return pool[x-1]; }

	template <class Ty>
	auto _push(Ty&& v, stack_type x)
	{
		auto new_node{free_nodes};
		if(free_nodes > pool.size()) 
		{
			pool.emplace_back(std::forward<Ty>(v), x);	
			++free_nodes;			
		} 
			else 
		{
			free_nodes = next(new_node);
			node(new_node) = std::move(
				node_t(std::forward<Ty>(v), x));
		}
		return new_node;
	}

public:
	stack_pool() 
		: free_nodes{1}
	{ 
	}

	explicit stack_pool(size_type n)
		: free_nodes{1}
	{
		reserve(n);
	}

	using iterator = _iterator<T, N>;
	using const_iterator = _iterator<const T, N>;
	friend iterator;
	friend const_iterator;

	stack_type new_stack() const noexcept { return _end; }

	iterator begin(stack_type x) { return iterator{this, x}; }
	iterator end(stack_type) noexcept { return iterator{this, _end}; }
	const_iterator begin(stack_type x) const { return const_iterator{this, x}; }
	const_iterator end(stack_type) const noexcept { return const_iterator{this, _end}; }
	const_iterator cbegin(stack_type x) const noexcept { return const_iterator{this, x}; }
	const_iterator cend(stack_type) const { return const_iterator{this, _end}; }

	stack_type end() const noexcept { return _end; }

	void reserve(stack_type n) { pool.reserve(n); }

	size_type capacity() const noexcept { return pool.capacity(); }

	bool empty() const noexcept { return capacity() == 0; }
	bool empty(stack_type x) const noexcept { return x == _end; }

	stack_type push(const T& v, stack_type i) { return _push(v, i); }
	stack_type push(T&& v, stack_type i) { return _push(std::move(v), i); }

	stack_type pop(stack_type x)
	{
		stack_type prev = node(x).next;
		node(x) = node_t{value_type{}, free_nodes};
		free_nodes = x;
		return prev;
	}

	// using libraries one may catch segmentation fault exceptions
	const T& value(stack_type i) const noexcept { return node(i).value; }
	T& value(stack_type i) noexcept { return node(i).value; }

	stack_type next(stack_type x) noexcept
	{ 
		// standard library, with few exception, does not throw exceptions 	
		// if (x == _end) throw std::runtime_error{"stack_pool::next(stack_type)"};
		return node(x).next; 
	}

	stack_type free_stack(stack_type x)
	{
		while(x != _end) 
			x = pop(x);
		return x;
	}

	stack_type fnodes_stack() const noexcept { return free_nodes; }
};

template <class T, class N>
constexpr N stack_pool<T, N>::_end;

template <class T, class N> 
class _iterator
{
private:
	using pool_type = stack_pool<T, N>;
	using stack_type = typename pool_type::stack_type;
	
	pool_type* current_pool;	
	stack_type current;

	friend pool_type;

	_iterator(pool_type* p, stack_type x)
		: current_pool{p}, current{x}
	{ }

public:
	using value_type = T;
	using reference = value_type&;
	using pointer = value_type*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;
	
	reference operator*() const noexcept { return current_pool->value(current); }

	_iterator& operator++()
	{
		current = current_pool->next(current);	
		return *this;
	}

	_iterator operator++(int)
	{
		auto tmp{*this};
		++(*this);
		return tmp;
	}

	friend bool operator==(const _iterator& x, const _iterator& y) noexcept
		{ return (x.current == y.current) && (x.current_pool == y.current_pool); }

	friend bool operator!=(const _iterator& x, const _iterator& y) noexcept
		{ return !(x == y); }
};
