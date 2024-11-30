#pragma once

#include "mono_config.h"
#include "mono_domain.h"
#include "mono_object.h"
#include "mono_type_traits.h"
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace mono
{

class mono_array_base : public mono_object
{
public:
	// using mono_object::mono_object;
	//  Construct from an existing MonoArray
	explicit mono_array_base(MonoArray* arr)
		: mono_object(reinterpret_cast<MonoObject*>(arr))
	{
	}

	explicit mono_array_base(const mono_object& obj)
		: mono_object(obj)
	{
	}

	// Get the size of the array
	auto size() const -> size_t
	{
		return mono_array_length(get_internal_array());
	}

protected:
	auto get_internal_array() const -> MonoArray*
	{
		return reinterpret_cast<MonoArray*>(object_);
	}
};

template <typename T>
class mono_array : public mono_array_base
{
public:
	using mono_array_base::mono_array_base;

	static_assert(is_mono_valuetype<T>::value, "Specialize mono_array for non-value types");

	auto craete_array(const mono_domain& domain, const std::vector<T>& vec) -> MonoArray*
	{
		auto type = mono_class_from_type();
		if(type)
		{
			return mono_array_new(domain.get_internal_ptr(), type, vec.size());
		}
		else
		{
			use_raw_bytes_ = true;
			return mono_array_new(domain.get_internal_ptr(), mono_get_byte_class(), vec.size() * sizeof(T));
		}
	}

	// Construct a new MonoArray from a std::vector of trivial types
	mono_array(const mono_domain& domain, const std::vector<T>& vec)
		: mono_array_base(craete_array(domain, vec))
	{
		for(size_t i = 0; i < vec.size(); ++i)
		{
			set(i, vec[i]);
		}
	}

	// Access element at index for trivial types
	auto get(size_t index) const -> T
	{
		if(use_raw_bytes_)
		{
			uint8_t* byte_ptr =
				reinterpret_cast<uint8_t*>(mono_array_addr_with_size(get_internal_array(), 1, 0));

			T value{};
			std::memcpy(&value, byte_ptr + index * sizeof(T), sizeof(T));
			return value;
		}
		else
		{
			// Directly retrieve primitive types
			return mono_array_get(get_internal_array(), T, index);
		}
	}

	// Set element at index for trivial types
	void set(size_t index, const T& value)
	{
		if(use_raw_bytes_)
		{
			uint8_t* byte_ptr =
				reinterpret_cast<uint8_t*>(mono_array_addr_with_size(get_internal_array(), 1, 0));
			std::memcpy(byte_ptr + index * sizeof(T), &value, sizeof(T));
		}
		else
		{
			// Directly set primitive types
			mono_array_set(get_internal_array(), T, index, value);
		}
	}

	// Convert to std::vector for trivial types
	auto to_vector() const -> std::vector<T>
	{
		std::vector<T> vec(size());
		for(size_t i = 0; i < size(); ++i)
		{
			vec[i] = get(i);
		}
		return vec;
	}

private:
	// Helper to get MonoClass for T without if constexpr
	static MonoClass* mono_class_from_type()
	{
		MonoClass* mono_type = nullptr;
		if(std::is_same<T, int8_t>::value)
		{
			mono_type = mono_get_sbyte_class();
		}
		if(std::is_same<T, int16_t>::value)
		{
			mono_type = mono_get_int16_class();
		}
		if(std::is_same<T, int32_t>::value)
		{
			mono_type = mono_get_int32_class();
		}
		if(std::is_same<T, int64_t>::value)
		{
			mono_type = mono_get_int64_class();
		}
		if(std::is_same<T, uint8_t>::value)
		{
			mono_type = mono_get_byte_class();
		}
		if(std::is_same<T, uint16_t>::value)
		{
			mono_type = mono_get_uint16_class();
		}
		if(std::is_same<T, uint32_t>::value)
		{
			mono_type = mono_get_uint32_class();
		}
		if(std::is_same<T, uint64_t>::value)
		{
			mono_type = mono_get_uint64_class();
		}
		else if(std::is_same<T, float>::value)
		{
			mono_type = mono_get_single_class();
		}
		else if(std::is_same<T, double>::value)
		{
			mono_type = mono_get_double_class();
		}
		else if(std::is_same<T, bool>::value)
		{
			mono_type = mono_get_boolean_class();
		}
		else if(std::is_same<T, char>::value)
		{
			mono_type = mono_get_char_class();
		}
		else
		{
			// mono_type = mono_get_byte_class();
		}
		return mono_type;
	}

	bool use_raw_bytes_{};
};

template <>
class mono_array<mono_object> : public mono_array_base
{
public:
	using mono_array_base::mono_array_base;

	// Construct a new MonoArray from a std::vector of primitive types
	mono_array(const mono_domain& domain, const std::vector<mono_object>& vec)
		: mono_array_base(vec.empty() ? nullptr
									  : mono_array_new(domain.get_internal_ptr(),
													   vec[0].get_type().get_internal_ptr(), vec.size()))
	{
		for(size_t i = 0; i < vec.size(); ++i)
		{
			set(i, vec[i]);
		}
	}

	// Access element at index for primitive types
	auto get(size_t index) const -> mono_object
	{
		MonoObject* obj = mono_array_get(get_internal_array(), MonoObject*, index);
		return mono_object(obj);
	}

	// Set element at index for primitive types
	void set(size_t index, const mono_object& value)
	{
		mono_array_set(get_internal_array(), MonoObject*, index, value.get_internal_ptr());
	}

	// Convert to std::vector for primitive types
	auto to_vector() const -> std::vector<mono_object>
	{
		std::vector<mono_object> vec(size());
		for(size_t i = 0; i < size(); ++i)
		{
			vec[i] = get(i);
		}
		return vec;
	}
};

} // namespace mono
