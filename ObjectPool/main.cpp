#include "ObjectPool.hpp"
#define _CRTDBG_MAP_ALLOC
#include <iostream>
#include <crtdbg.h>

int main()
{
	const char* test_c_str = "Everyone hunting you. Watch out!";
	std::string test_str{ test_c_str };
	std::vector<char> str_vector(test_str.begin(), test_str.end());
	auto moved_str(test_str);

	ObjectPool<std::string> str_pool(4);
	auto& str_1 = str_pool.allocate(test_c_str);
	auto& str_2 = str_pool.allocate(test_str);
	auto& str_3 = str_pool.allocate(str_vector.begin(), str_vector.end());
	auto& str_4 = str_pool.allocate(move(moved_str));
	if (str_1 != test_str) {
		std::cout << "Something went wrong\n";
	}
	if (str_2 != test_str) {
		std::cout << "Something went wrong\n";
	}
	if (str_3 != test_str) {
		std::cout << "Something went wrong\n";
	}
	if (str_4 != test_str) {
		std::cout << "Something went wrong\n";
	}
	if (!moved_str.empty()) {
		std::cout << "Something went wrong\n";
	}
	return _CrtDumpMemoryLeaks();;
}
