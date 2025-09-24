#pragma once
#include "mono_field.h"

#include "mono_object.h"
#include "mono_type_conversion.h"
namespace mono
{

template <typename T>
class mono_field_invoker : public mono_field
{
public:
	using value_type = T;

	void set_value(const T& val) const;

	void set_value(const mono_object& obj, const T& val) const;

	auto get_value() const -> T;

	auto get_value(const mono_object& obj) const -> T;

private:
	template <typename signature_t>
	friend auto make_field_invoker(const mono_field&) -> mono_field_invoker<signature_t>;

	explicit mono_field_invoker(const mono_field& field)
		: mono_field(field)
	{
	}

	void set_value_impl(const mono_object* obj, const T& val) const;

	auto get_value_impl(const mono_object* obj) const -> T;
};

template <typename T>
void mono_field_invoker<T>::set_value(const T& val) const
{
	set_value_impl(nullptr, val);
}

template <typename T>
void mono_field_invoker<T>::set_value(const mono_object& object, const T& val) const
{
	set_value_impl(&object, val);
}

template <typename T>
void mono_field_invoker<T>::set_value_impl(const mono_object* object, const T& val) const
{
	assert(field_);

	auto mono_val = mono_converter<T>::to_mono(val);
	auto arg = to_mono_arg(mono_val);

	if(object)
	{
		auto obj = object->get_internal_ptr();
		assert(obj);
		mono_field_set_value(obj, field_, arg);
	}
	else
	{
		mono_field_static_set_value(owning_type_vtable_, field_, arg);
	}
}

template <typename T>
auto mono_field_invoker<T>::get_value() const -> T
{
	return get_value_impl(nullptr);
}

template <typename T>
auto mono_field_invoker<T>::get_value(const mono_object& object) const -> T
{
	return get_value_impl(&object);
}

template <typename T>
auto mono_field_invoker<T>::get_value_impl(const mono_object* object) const -> T
{
	T val{};
	assert(field_);
	MonoObject* refvalue = nullptr;
	void* arg = &val;
	if(!is_valuetype())
	{
		arg = &refvalue;
	}
	if(object)
	{
		auto obj = object->get_internal_ptr();
		assert(obj);
		mono_field_get_value(obj, field_, arg);
	}
	else
	{
		mono_field_static_get_value(owning_type_vtable_, field_, arg);
	}

	if(!is_valuetype())
	{
		val = mono_converter<T>::from_mono(refvalue);
	}
	return val;
}

template <typename T>
auto make_field_invoker(const mono_field& field) -> mono_field_invoker<T>
{
	return mono_field_invoker<T>(field);
}

template <typename T>
auto make_field_invoker(const mono_type& type, const std::string& name) -> mono_field_invoker<T>
{
	auto field = type.get_field(name);
	return make_field_invoker<T>(field);
}

template <typename T>
auto make_invoker(const mono_field& field) -> mono_field_invoker<T>
{
	return make_field_invoker<T>(field);
}

template <typename T>
auto set_field_value(const mono_object& obj, const std::string& name, const T& val) -> bool
{
	try
	{
		auto invoker = make_field_invoker<T>(obj.get_type(), name);
		invoker.set_value(obj, val);
		return true;
	}
	catch(const std::exception& e)
	{
		return false;
	}
}


template <typename T>
auto set_field_value(const mono_type& type, const std::string& name, const T& val) -> bool
{
	try
	{
		auto invoker = make_field_invoker<T>(type, name);
		invoker.set_value(val);
		return true;
	}
	catch(const std::exception& e)
	{
		return false;
	}
}

template <typename T>
auto get_field_value(const mono_object& obj, const std::string& name, T& val) -> bool
{
	try
	{
		auto invoker = make_field_invoker<T>(obj.get_type(), name);
		val = invoker.get_value(obj);
		return true;
	}
	catch(const std::exception& e)
	{
		return false;
	}
}

template <typename T>
auto get_field_value(const mono_type& type, const std::string& name, T& val) -> bool
{
	try
	{
		auto invoker = make_field_invoker<T>(type, name);
		val = invoker.get_value();
		return true;
	}
	catch(const std::exception& e)
	{
		return false;
	}
}
} // namespace mono
