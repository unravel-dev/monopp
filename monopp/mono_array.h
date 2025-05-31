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

	auto get_element_type() const -> mono_type
	{
		return mono_type(get_element_type(get_internal_array()));
	}

	virtual auto get_object(size_t index) const -> mono_object = 0;

protected:
	auto get_internal_array() const -> MonoArray*
	{
		return reinterpret_cast<MonoArray*>(object_);
	}

	static MonoClass* get_element_type(MonoArray* array)
	{
		if(!array)
			return nullptr; // Safety check

		// Get the MonoClass* of the array itself
		MonoClass* arrayClass = mono_object_get_class((MonoObject*)array);

		// Get the element type class
		MonoClass* elementType = mono_class_get_element_class(arrayClass);

		return elementType; // This is the class of the elements inside the array
	}
};

template <typename T>
class mono_array : public mono_array_base
{
public:
	using mono_array_base::mono_array_base;

	static_assert(is_mono_valuetype<T>::value, "Specialize mono_array for non-value types");

	auto craete_array(const mono_domain& domain, size_t count, const mono_type& element_type)
		-> MonoArray*
	{
		auto element_type_ = element_type.get_internal_ptr();

		if(!element_type_)
		{
			element_type_ = mono_class_from_type();
		}

		if(element_type_)
		{
			return mono_array_new(domain.get_internal_ptr(), element_type_, count);
		}
		else
		{
			use_raw_bytes_ = true;
			return mono_array_new(domain.get_internal_ptr(), mono_get_byte_class(), count * sizeof(T));
		}
	}

	// Construct a new MonoArray from a std::VectorLike of trivial types
	template<typename VectorLike = std::vector<T>>
	mono_array(const mono_domain& domain, const VectorLike& vec)
		: mono_array_base(craete_array(domain, vec.size(), {}))
	{
		for(size_t i = 0; i < vec.size(); ++i)
		{
			set(i, vec[i]);
		}
	}

	template<typename VectorLike = std::vector<T>>
	mono_array(const mono_domain& domain, const VectorLike& vec, const mono_type& element_type)
		: mono_array_base(craete_array(domain, vec.size(), element_type))
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

	auto get_object(size_t index) const -> mono_object
	{
		MonoObject* object = nullptr;
		auto type = get_element_type();
		if(type.is_valuetype())
		{
			auto val = get(index);
			object = mono_value_box(mono_object_get_domain(object_), type.get_internal_ptr(), &val);
		}
		else
		{
			object = mono_array_get(get_internal_array(), MonoObject*, index);
		}
		return mono_object(object);
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

	// Convert to std::VectorLike for trivial types
	template<typename VectorLike = std::vector<T>>
	auto to_vector() const -> VectorLike
	{
		VectorLike vec(size());
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
	template<typename VectorLike = std::vector<mono_object>>
	mono_array(const mono_domain& domain, const VectorLike& vec)
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

	auto get_object(size_t index) const -> mono_object
	{
		return get(index);
	}

	// Set element at index for primitive types
	void set(size_t index, const mono_object& value)
	{
		mono_array_set(get_internal_array(), MonoObject*, index, value.get_internal_ptr());
	}

	// Convert to std::vector like for primitive types
	template<typename VectorLike = std::vector<mono_object>>
	auto to_vector() const -> VectorLike
	{
		VectorLike vec(size());
		for(size_t i = 0; i < size(); ++i)
		{
			vec[i] = get(i);
		}
		return vec;
	}
};

} // namespace mono
