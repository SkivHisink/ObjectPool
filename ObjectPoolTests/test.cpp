#include "pch.h"
#include <array>
#include <random>

#include "objectPool.hpp"

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
		std::mt19937 mersenne(0);
		return charset[mersenne() % max_index];
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
struct Standart_tests : MemoryLeaksTests {};
class Atypical_tests : public MemoryLeaksTests {};

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
	bool operator==(const passiveDataStructure& other) const
	{
		return a == other.a && b == other.b && c == other.c;
	}
};

TEST_F(Standart_tests, BigObjectsCount)
{
	constexpr size_t objects_count = 1027;
	constexpr size_t max_strLen = 2020;

	ObjectPool<std::string> str_pool(objects_count);

	std::vector<std::string> expected_strings(objects_count);
	generate(expected_strings.begin(), expected_strings.end(),
		random_string_generator(max_strLen));

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

TEST_F(Standart_tests, DifferentConstructors)
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

TEST_F(Standart_tests, PODtype)
{
	constexpr size_t objects_count = 4;

	passiveDataStructure expected_1 = { 1, 4.0, 'a' };
	passiveDataStructure expected_2 = { 2, 5.0, 'b' };
	passiveDataStructure expected_3 = { 3, 6.0, 'c' };
	passiveDataStructure expected_4 = { 4, 7.0, 'd' };

	ObjectPool< passiveDataStructure> pod_pool(objects_count);

	auto& tested_pod_1 = pod_pool.allocate(1, 4.0, 'a');
	auto& tested_pod_2 = pod_pool.allocate(2, 5.0, 'b');
	auto& tested_pod_3 = pod_pool.allocate(3, 6.0, 'c');
	auto& tested_pod_4 = pod_pool.allocate(4, 7.0, 'd');
	EXPECT_THROW(pod_pool.allocate(5, 7.0, 'd'), std::logic_error);
	EXPECT_EQ(expected_1, tested_pod_1);
	EXPECT_EQ(expected_2, tested_pod_2);
	EXPECT_EQ(expected_3, tested_pod_3);
	EXPECT_EQ(expected_4, tested_pod_4);
	pod_pool.free(tested_pod_4);
	pod_pool.free(tested_pod_3);
	pod_pool.free(tested_pod_2);
	pod_pool.free(tested_pod_1);
}

TEST_F(Standart_tests, TooManyObjects)
{
	constexpr size_t max_obj_count = 100;
	constexpr size_t string_size = 100;

	ObjectPool<std::string> str_pool(max_obj_count);

	for (size_t i = 0; i < max_obj_count; ++i)
	{
		str_pool.allocate(random_string(string_size));
	}

	EXPECT_THROW(str_pool.allocate(random_string(string_size)), std::exception);
}
TEST_F(Standart_tests, NonClassTest)
{
	ObjectPool<int> pool(5);
	auto& test_int_1 = pool.allocate(1);
	auto& test_int_2 = pool.allocate(2);
	auto& test_int_3 = pool.allocate(3);
	auto& test_int_4 = pool.allocate(4);
	auto& test_int_5 = pool.allocate(5);
	EXPECT_THROW(pool.allocate(6), std::logic_error);
	EXPECT_EQ(test_int_1, 1);
	EXPECT_EQ(test_int_2, 2);
	EXPECT_EQ(test_int_3, 3);
	EXPECT_EQ(test_int_4, 4);
	EXPECT_EQ(test_int_5, 5);
	pool.free(test_int_1);
	pool.free(test_int_2);
	pool.free(test_int_3);
	pool.free(test_int_4);
	pool.free(test_int_5);
}
TEST_F(Atypical_tests, ExceptionInConstructor)
{
	ObjectPool<Tested> tested_pool(3);

	EXPECT_THROW(tested_pool.allocate(3), std::exception);

	for (size_t i = 0; i < 3; ++i)
	{
		EXPECT_NO_THROW(tested_pool.allocate());
	}
}

TEST_F(Atypical_tests, FreeObjetNotFromPool)
{
	std::string str = "wrong str!";

	ObjectPool<std::string> str_pool(3);

	auto& tested = str_pool.allocate("123");

	EXPECT_THROW(str_pool.free(str), std::exception);
}

TEST_F(Atypical_tests, NonObjectsTest)
{
	ObjectPool<std::string> str_pool(0);

	EXPECT_THROW(str_pool.allocate("123\n"), std::exception);
}
class Except_summoner
{
public:
	Except_summoner() {}
	template<typename T>
	Except_summoner(T anything)
	{
		throw std::exception("Except_summoner non default constructor was called with one arg");
	}
};

TEST_F(Standart_tests, NonStdClassTest)
{
	ObjectPool<Except_summoner> pool(10);

	EXPECT_THROW(pool.allocate("Zero problems"), std::exception);
	std::vector<int> f;
	f.push_back(32);
	EXPECT_THROW(pool.allocate(f), std::exception);
	EXPECT_THROW(pool.allocate(42.58), std::exception);

	EXPECT_NO_THROW(pool.allocate());
}
TEST_F(Standart_tests, RandomItemDeleting)
{
	constexpr size_t objects_count = 1027;
	constexpr size_t max_strLen = 2020;

	ObjectPool<std::string> str_pool(objects_count);

	std::vector<std::string> expected_strings(objects_count);
	generate(expected_strings.begin(), expected_strings.end(),
		random_string_generator(max_strLen));

	std::vector<std::reference_wrapper<std::string>> tested_strings;
	tested_strings.reserve(objects_count);

	for (auto const& expected_string : expected_strings)
	{
		tested_strings.emplace_back(str_pool.allocate(expected_string));
	}
	std::mt19937 mersenne(0);
	std::shuffle(tested_strings.begin(), tested_strings.end(), mersenne);
	for (auto i = 0; i < objects_count; ++i)
	{
		str_pool.free(tested_strings[i]);
	}
}
int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}