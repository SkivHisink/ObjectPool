#include "pch.h"
#include <array>
#include "objectPool.hpp"

namespace func_for_test {
	class MemoryLeaksTests : public ::testing::Test {
	protected:
		void SetUp() override {
			_CrtMemCheckpoint(&startup_);
		}
		void TearDown() override {
			_CrtMemState teardown, diff;
			_CrtMemCheckpoint(&teardown);
			ASSERT_EQ(0, _CrtMemDifference(&diff, &startup_, &teardown)) << "Memory leaks was detected";
		}
		_CrtMemState startup_ = {};
	};

	std::string random_string(size_t length)
	{
		auto rand_letter = []()
		{
			const char charset[] =
				"123"
				"321"
				"1231"
				"How does it work"
				"test"
				"beta test";

			const auto max_index = (sizeof(charset) - 1);
			return charset[rand() % max_index];
		};
		std::string str(length, 0);
		std::generate_n(str.begin(), length, rand_letter);
		return str;
	}

	struct random_string_generator
	{
		size_t length = 0;

		explicit random_string_generator(size_t first_length) : length(first_length)
		{}

		std::string operator()()
		{
			const auto prev_length = length;
			length += 2;

			return random_string(prev_length);
		}
	};
	struct STANDART_TESTS : MemoryLeaksTests {};
	class ATYPICAL_TESTS : public MemoryLeaksTests {};

	class Tested
	{

	public:
		Tested() = default;

		Tested(int a)
		{
			throw std::exception("constructor of Tested class exception");
		}
	};
	struct passiveDataStructure
	{
		int a;
		double b;
		char c;
	};
}

TEST(STANDART_TESTS, BigObjectsCount)
{
	constexpr size_t objects_count = 1027;
	constexpr size_t max_strLen = 2020;

	ObjectPool<std::string> str_pool(objects_count);

	std::vector<std::string> expected_strings(objects_count);
	generate(expected_strings.begin(), expected_strings.end(),
		func_for_test::random_string_generator(max_strLen));

	std::vector<std::reference_wrapper<std::string>> tested_strings;
	tested_strings.reserve(objects_count);

	for (auto const& expected_string : expected_strings)
	{
		tested_strings.emplace_back(str_pool.allocate(expected_string));
	}

	EXPECT_TRUE(
		equal(expected_strings.begin(), expected_strings.end(), tested_strings.begin(),
			[](auto& x, auto& y)
			{
				return x == y.get();
			}
		)
	);
}

TEST(STANDART_TESTS, DifferentConstructors)
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

	EXPECT_EQ(str_1, test_str);
	EXPECT_EQ(str_2, test_str);
	EXPECT_EQ(str_3, test_str);
	EXPECT_EQ(str_4, test_str);
	EXPECT_TRUE(moved_str.empty());
}
TEST(STANDART_TESTS, DifficultClass)
{
	constexpr size_t objects_count = 64;
	constexpr int objects_in_map_count = 64;
	constexpr size_t max_str_len = 100;
	func_for_test::random_string_generator str_gen(max_str_len);

	using test_map = std::map<std::string, int>;

	ObjectPool<test_map> map_pool(objects_count);

	std::vector<test_map> expected_maps(objects_count);
	std::vector<std::reference_wrapper<test_map>> tested_maps;
	tested_maps.reserve(objects_count);

	for (auto& expected_map : expected_maps)
	{
		for (auto i = 0; i < objects_in_map_count; ++i)
		{
			expected_map.emplace(str_gen(), rand() % objects_in_map_count);
		}
	}

	for (auto const& expected_map : expected_maps)
	{
		tested_maps.emplace_back(map_pool.allocate(expected_map));
	}

	EXPECT_TRUE(equal(expected_maps.begin(), expected_maps.end(), tested_maps.begin(),
		[](auto& x, auto& y)
		{
			return x == y.get();
		}
	));
}
TEST(STANDART_TESTS, FreeTest)
{
	constexpr size_t objects_count = 1024;
	constexpr size_t max_str_len = 512;

	ObjectPool<std::string> str_pool(objects_count);

	std::vector<std::string> expected_strings(objects_count);
	generate(expected_strings.begin(), expected_strings.end(),
		func_for_test::random_string_generator(max_str_len));

	std::vector<std::string> garbage_strings(objects_count);
	generate(expected_strings.begin(), expected_strings.end(),
		func_for_test::random_string_generator(max_str_len + 1));

	std::vector<std::reference_wrapper<std::string>> tested_strings;
	tested_strings.reserve(objects_count);


	for (auto const& garbage_string : garbage_strings)
	{
		EXPECT_NO_THROW(tested_strings.emplace_back(str_pool.allocate(garbage_string)));
	}

	for (auto it = tested_strings.rbegin(); it != tested_strings.rend(); ++it)
	{
		str_pool.free(it->get());
	}

	tested_strings.clear();

	for (auto const& expected_string : expected_strings)
	{
		EXPECT_NO_THROW(tested_strings.emplace_back(str_pool.allocate(expected_string)));
	}

	EXPECT_TRUE(
		equal(expected_strings.begin(), expected_strings.end(), tested_strings.begin(),
			[](auto& x, auto& y)
			{
				return x == y.get();
			}
		)
	);

}
TEST(STANDART_TESTS, NonClassObjects)
{
	constexpr size_t objects_count = 2048;

	ObjectPool<int> str_pool(objects_count);

	std::vector<int> expected_ints(objects_count);
	generate(
		expected_ints.begin(),
		expected_ints.end(),
		[]() { return rand(); }
	);

	std::vector<int> tested_ints(objects_count);

	auto expected_it = expected_ints.begin();

	generate(tested_ints.begin(), tested_ints.end(),
		[&expected_it]()
		{
			return *(expected_it++);
		}
	);

	EXPECT_TRUE(
		equal(expected_ints.begin(), expected_ints.end(), tested_ints.begin())
	);
}
TEST(STANDART_TESTS, PODtype)
{
	constexpr size_t objects_count = 4;

	func_for_test::passiveDataStructure expected_1 = { 1, 4.0, 'a' };
	func_for_test::passiveDataStructure expected_2 = { 2, 5.0, 'b' };
	func_for_test::passiveDataStructure expected_3 = { 3, 6.0, 'c' };
	func_for_test::passiveDataStructure expected_4 = { 4, 7.0, 'd' };

	ObjectPool<func_for_test::passiveDataStructure> pod_pool(objects_count);

	auto& tested_pod_1 = pod_pool.allocate(1, 4.0, 'a');
	auto& tested_pod_2 = pod_pool.allocate(2, 5.0, 'b');
	auto& tested_pod_3 = pod_pool.allocate(3, 6.0, 'c');
	auto& tested_pod_4 = pod_pool.allocate(4, 7.0, 'd');

	pod_pool.free(tested_pod_4);
	pod_pool.free(tested_pod_3);
	pod_pool.free(tested_pod_2);
	pod_pool.free(tested_pod_1);
}

TEST(STANDART_TESTS, TooManyObjects)
{
	constexpr size_t max_obj_count = 100;
	constexpr size_t string_size = 100;

	ObjectPool<std::string> str_pool(max_obj_count);

	for (size_t i = 0; i < max_obj_count; ++i)
	{
		str_pool.allocate(func_for_test::random_string(string_size));
	}

	EXPECT_THROW(str_pool.allocate(func_for_test::random_string(string_size)), std::exception);
}

TEST(ATYPICAL_TESTS, ExceptionInConstructor)
{
	ObjectPool<func_for_test::Tested> tested_pool(3);

	EXPECT_THROW(tested_pool.allocate(3), std::exception);

	for (size_t i = 0; i < 3; ++i)
	{
		EXPECT_NO_THROW(tested_pool.allocate());
	}
}

TEST(ATYPICAL_TESTS, FreeObjetNotFromPool)
{
	std::string str = "wrong str!";

	ObjectPool<std::string> str_pool(3);

	auto& tested = str_pool.allocate("123");

	EXPECT_THROW(str_pool.free(str), std::exception);
}

TEST(ATYPICAL_TESTS, NonObjectsTest)
{
	ObjectPool<std::string> str_pool(0);

	EXPECT_THROW(str_pool.allocate("123\n"), std::exception);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}