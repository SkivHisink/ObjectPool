#pragma once
#include <memory>
#include <vector>
#include <stdexcept>
#include <stack>

template<class Object>
class ObjectPool final
{
public:
	ObjectPool(ObjectPool&&) noexcept = default;

	ObjectPool(ObjectPool const&) = delete;
	ObjectPool& operator=(ObjectPool const&) = delete;
	ObjectPool& operator=(ObjectPool&&) noexcept = default;

	explicit ObjectPool(size_t pool_size) : data(new uint8_t[pool_size * sizeof(Object)]), free_object_flags(pool_size, true), capacity(pool_size)
	{
		for (size_t i = 0; i < pool_size; ++i) {
			free_objects.push_back(get_object_ptr(i));
		}
	}

	template <typename ... Args>
	Object& allocate(Args&& ...args)
	{
		if (free_objects.empty()) {
			throw std::logic_error("Error in method allocate() of class ObjectPool. Memory can't be allocated for a new object in the pool by some reasons");
		}

		Object* object_ptr = free_objects[free_objects.size() - 1];
		new(object_ptr) Object{ std::forward<Args>(args)... };

		free_object_flags[object_ptr - get_object_ptr(0)] = false;
		free_objects.pop_back();

		return *object_ptr;
	}

	void  free(Object& object)
	{
		Object* object_ptr = &object;

		if (object_ptr < get_object_ptr(0) || object_ptr > get_object_ptr(capacity - 1)) {
			throw std::logic_error("Error in method free() of class ObjectPool. You can't free objects from this pool.");
		}
		if constexpr (std::is_class_v<Object> && !std::is_pod_v<Object>) {
			object.~Object();
		}

		free_object_flags[object_ptr - get_object_ptr(0)] = true;
		free_objects.push_back(object_ptr);
	}

	~ObjectPool()
	{
		if constexpr (std::is_class_v<Object> && !std::is_pod_v<Object>) {
			for (size_t i = 0; i < capacity; ++i) {
				if (!free_object_flags[i]) {
					get_object_ptr(i)->~Object();
				}
			}
		}
	}
private:
	Object* get_object_ptr(size_t position) noexcept { return reinterpret_cast<Object*>(data.get() + position * sizeof(Object)); }
	std::unique_ptr<uint8_t[]> data;
	std::vector<Object*> free_objects;
	std::vector<bool> free_object_flags;
	size_t capacity;
};