#pragma once

#include "mono_config.h"

#include "mono_array.h"
#include "mono_list.h"
#include "mono_domain.h"
#include "mono_string.h"
#include "mono_type.h"
#include "mono_type_traits.h"

namespace mono
{

template <typename T>
inline auto to_mono_arg(T& t)
{
	static_assert(is_mono_valuetype<T>::value, "Should not pass here for non-pod types");
	return std::addressof(t);
}
inline auto to_mono_arg(MonoObject* t)
{
	return t;
}

template <typename T>
auto check_type_layout(MonoObject* obj) -> bool
{
	mono_object object(obj);
	const auto& type = object.get_type();
	const auto mono_sz = type.get_sizeof();
	const auto mono_align = type.get_alignof();
	constexpr auto cpp_sz = sizeof(T);
	constexpr auto cpp_align = alignof(T);

	return mono_sz == cpp_sz && mono_align <= cpp_align;
}

template <typename T>
struct mono_converter
{
	using native_type = T;
	using managed_type = T;

	static_assert(is_mono_valuetype<managed_type>::value, "Specialize converter for non-value types");

	static auto to_mono(const native_type& obj) -> managed_type
	{
		return obj;
	}

	template <typename U>
	static auto from_mono(U obj) -> std::enable_if_t<std::is_same<U, MonoObject*>::value, native_type>
	{
		assert(check_type_layout<managed_type>(obj) && "Different type layouts");
		void* ptr = mono_object_unbox(obj);
		return *reinterpret_cast<native_type*>(ptr);
	}

	template <typename U>
	static auto from_mono(const U& obj)
		-> std::enable_if_t<!std::is_same<U, MonoObject*>::value && !std::is_pointer<U>::value, const native_type&>
	{
		return obj;
	}

	template <typename U>
	static auto from_mono(const U& ptr)
		-> std::enable_if_t<!std::is_same<U, MonoObject*>::value && std::is_pointer<U>::value, native_type*>
	{
		return reinterpret_cast<native_type*>(ptr);
	}
};

template <>
struct mono_converter<mono_object>
{
	using native_type = mono_object;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		return obj.get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return native_type(obj);
	}
};

template <>
struct mono_converter<mono_type>
{
	using native_type = mono_type;
	using managed_type = MonoReflectionType*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		// Get the current Mono domain
		MonoDomain* domain = mono_domain_get();

		// Obtain the MonoType* from your native_type (assuming a getter function)
		MonoType* monoType = mono_class_get_type(obj.get_internal_ptr());

		// Convert MonoType* to MonoReflectionType*
		MonoReflectionType* reflectionType = mono_type_get_object(domain, monoType);

		return reflectionType;
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		MonoType* monoType = mono_reflection_type_get_type(obj);
		return native_type(monoType);
	}
};

template <>
struct mono_converter<std::string>
{
	using native_type = std::string;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		const auto& domain = mono_domain::get_current_domain();
		return mono_string(domain, obj).get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return mono_string(mono_object(obj)).as_utf8();
	}
};

template <typename T>
struct mono_converter<std::vector<T>>
{
	using native_type = std::vector<T>;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		const auto& domain = mono_domain::get_current_domain();
		return mono_array<T>(domain, obj).get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return mono_array<T>(mono_object(obj)).to_vector();
	}
};

template <typename T>
struct mono_converter<mono_array<T>>
{
	using native_type = mono_array<T>;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		return obj.get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return mono_array<T>(mono_object(obj));
	}
};


template <typename T>
struct mono_converter<std::list<T>>
{
	using native_type = std::list<T>;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		const auto& domain = mono_domain::get_current_domain();
		return mono_list<T>(domain, obj).get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return mono_list<T>(mono_object(obj)).to_list();
	}
};

template <typename T>
struct mono_converter<mono_list<T>>
{
	using native_type = mono_list<T>;
	using managed_type = MonoObject*;

	static auto to_mono(const native_type& obj) -> managed_type
	{
		return obj.get_internal_ptr();
	}

	static auto from_mono(const managed_type& obj) -> native_type
	{
		if(!obj)
		{
			return {};
		}
		return mono_list<T>(mono_object(obj));
	}
};


} // namespace mono
