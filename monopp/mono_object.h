#pragma once

#include "mono/metadata/appdomain.h"
#include "mono_config.h"
#include "mono_type.h"
#include "mono_type_traits.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/object.h>
END_MONO_INCLUDE

namespace mono
{
class mono_domain;

class mono_object
{
public:
	mono_object();
	explicit mono_object(MonoObject* object);
	explicit mono_object(MonoObject* object, const mono_type& type);

	explicit mono_object(const mono_domain& domain, const mono_type& type);

	auto get_type() const -> const mono_type&;

	auto valid() const -> bool;
	operator bool() const;
	
	auto is_valid_mono_object() const -> bool;

	auto get_internal_ptr() const -> MonoObject*;

	
	template<typename T>
	void box_value(const T& value, const mono_type& type)
	{
		static_assert(is_mono_valuetype<T>::value, "Should not pass here for non-value types");
		void* ptr = const_cast<T*>(std::addressof(value));
		object_ = mono_value_box(mono_domain_get(), type.get_internal_ptr(), ptr);
		type_ = type;
	}

	template<typename T>
	auto unbox_value() const -> T
	{
		static_assert(is_mono_valuetype<T>::value, "Should not pass here for non-value types");
		void* ptr = mono_object_unbox(get_internal_ptr());
		return *reinterpret_cast<T*>(ptr);
	}

	void set_value(MonoObject* object, const mono_type& type)
    {
        object_ = object;
        type_ = type;
    }

	void set_value(MonoObject* object)
    {
        object_ = object;
		if(object_)
		{
			type_ = mono_type(mono_object_get_class(object));
		}    
	}
protected:



	mono_type type_;

	non_owning_ptr<MonoObject> object_ = nullptr;
};

} // namespace mono
