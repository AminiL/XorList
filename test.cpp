#include "StackAllocator.h"
#include "XorList.h"
#include <iostream>
#include <list>
#include <vector>
#include <iterator>
#include <set>
#include <random>
#include <iostream>
#include <list>
#include <chrono>
#include <algorithm>
template<class Callable, class... Args>
auto benchmark(Callable f, Args... args) {
	const auto t1 = std::chrono::high_resolution_clock::now();
	f(std::forward<Args>(args)...);
	const auto t2 = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
}
int main() {
	std::mt19937 gen((std::random_device()()));
	std::uniform_int_distribution<> op(0, 3),
		value;
	size_t test_length = std::uniform_int_distribution<>(1e4, 1e7)(gen);

	const XorList<int, StackAllocator<int>> const_brace_init_l(1, 2);  // test brace init constructor
	
	//const_brace_init_l.begin();  // test const iterators
	XorList<int, StackAllocator<int>> fill_l(5, 0);  // test filling constructor
													 // test move constructor
	const auto moved_to = std::move(fill_l);

	XorList<int, StackAllocator<int>> l;  // test default constructor
	std::list<int> std_list;
	try {
		std::cout << "test length: " << test_length << "\n";
		for (int i = 0; i < test_length; ++i) {
			switch (op(gen)) {
			case 0: l.push_back(value(gen)), std_list.push_back(l.back()); break;
			case 1: if (l.size()) l.pop_back(), std_list.pop_back(); break;
			case 2: l.push_front(value(gen)), std_list.push_front(l.front()); break;
			case 3: if (l.size()) l.pop_front(), std_list.pop_front(); break;
			}
		}
		// rval versions
		std_list.insert(std_list.begin(),
			*l.insert_before(l.begin(), value(gen)));
		std_list.insert(std_list.end(),
			*l.insert_after(std::prev(l.end()), value(gen)));
		// lval versions
		auto val = value(gen);
		std_list.insert(std_list.begin(),
			*l.insert_before(l.begin(), val));
		std_list.insert(std_list.end(),
			*l.insert_after(std::prev(l.end()), val));

		l.push_front(val);
		std_list.push_front(val);
		l.push_back(val);
		std_list.push_back(val);

		// erase from middle
		if (l.size()) {
			l.erase(std::next(l.begin(), std::min<size_t>(5, l.size() - 1)));
			std_list.erase(std::next(std_list.begin(), std::min<size_t>(5, std_list.size() - 1)));
		}
		if (l.size()) {
			l.erase(std::next(l.begin(), std::min<size_t>(25, l.size() - 1)));
			std_list.erase(std::next(std_list.begin(), std::min<size_t>(25, std_list.size() - 1)));
		}

		if (l.size()) l.pop_front(), std_list.pop_front();
		if (l.size()) l.pop_back(), std_list.pop_back();


		auto l_copy = l; // copy assignment

		std::cout << "correctness check: ";
		if (std::equal(l_copy.begin(), l_copy.end(), std_list.begin(), std_list.end())
			&& std::equal(l_copy.rbegin(), l_copy.rend(), std_list.rbegin(), std_list.rend())) {
			std::cout << "passed\n";
		}
		else std::cout << "failed\n";
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
		throw;
	}

	try {
		auto test = [test_length, &gen, &value, &op](auto list) {
			for (size_t i = 0; i < test_length; ++i) {
				switch (op(gen)) {
				case 0: list.push_back(value(gen)); break;
				case 1: if (list.size()) list.pop_back(); break;
				case 2: list.push_front(value(gen)); break;
				case 3: if (list.size()) list.pop_front(); break;
				}
			}
			list.clear();
		};
		std::cout << "std::allocator: " << benchmark(test, XorList<int>()) << " ms\n"
			<< "StackAllocator: " << benchmark(test, XorList<int, StackAllocator<int>>()) << " ms\n";

	}
	catch (const std::exception& e) {
		std::cerr << e.what();
		throw;
	}
}
