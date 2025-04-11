#include "mono_type.h"
#include "mono_assembly.h"
#include "mono_exception.h"

#include "mono_domain.h"
#include "mono_field.h"
#include "mono_method.h"
#include "mono_object.h"
#include "mono_property.h"
#include "mono_type_conversion.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/appdomain.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/debug-helpers.h>
END_MONO_INCLUDE
#include <iostream>
namespace mono
{
namespace
{
auto strip_namespace(const std::string& full_name) -> std::string
{
	std::string result;
	size_t start = 0;

	while(start < full_name.size())
	{
		// Find the next '<' or ',' to handle generic arguments
		size_t end = full_name.find_first_of("<,", start);

		// If not found, process the remainder
		if(end == std::string::npos)
		{
			end = full_name.size();
		}

		// Extract the segment (e.g., "Namespace.Type" or "Type")
		std::string segment = full_name.substr(start, end - start);

		// Remove everything before the last '.' (namespace separator)
		size_t last_dot = segment.find_last_of('.');
		if(last_dot != std::string::npos)
		{
			segment = segment.substr(last_dot + 1);
		}

		// Append the processed segment
		result += segment;

		// Add back the delimiter if it's not the end of the string
		if(end < full_name.size() && (full_name[end] == '<' || full_name[end] == ','))
		{
			result += full_name[end];
		}

		// Move to the next segment
		start = end + 1;
	}

	return result;
}

// This function returns a vector of pairs. Each pair is composed of the underlying integer value
// and its corresponding string representation from the enum type.
// 'domain' is the MonoDomain*, and 'enumClass' is the MonoClass* representing the enum type.
template <typename T>
std::vector<std::pair<T, std::string>> get_enum_options(MonoClass* enumClass)
{
	MonoDomain* domain = mono_domain_get();
	std::vector<std::pair<T, std::string>> options;
	if(!domain || !enumClass)
		return options; // Return an empty vector if invalid parameters.

	// Get the MonoType for the enum.
	MonoType* enumType = mono_class_get_type(enumClass);
	if(!enumType)
		return options;

	// Convert the MonoType into a reflection type (System.Type).
	MonoReflectionType* reflectionType = mono_type_get_object(domain, enumType);
	if(!reflectionType)
		return options;

	// Get the System.Enum class via the Mono API.
	MonoClass* systemEnumClass = mono_get_enum_class();
	if(!systemEnumClass)
		return options;

	// Locate the 'GetValues' method defined on System.Enum.
	// The expected signature is: public static Array GetValues(Type enumType)
	MonoMethod* getValuesMethod = mono_class_get_method_from_name(systemEnumClass, "GetValues", 1);
	if(!getValuesMethod)
		return options;

	// Prepare the argument: the reflection type (a System.Type) for our enum.
	void* args[1] = {reflectionType};

	// Invoke the method to retrieve the array of enum values.
	MonoObject* result = mono_runtime_invoke(getValuesMethod, nullptr, args, nullptr);
	if(!result)
		return options;

	// Convert the result to a MonoArray.
	MonoArray* enumArray = (MonoArray*)result;
	uintptr_t arrayLength = mono_array_length(enumArray);

	for(uintptr_t i = 0; i < arrayLength; i++)
	{
		// Retrieve the enum value object.
		T value = mono_array_get(enumArray, T, i);

		MonoObject* enumValueObj = mono_value_box(domain, enumClass, &value);

		// Get the string representation of the enum value by calling its ToString() method.
		MonoString* strObj = (MonoString*)mono_object_to_string(enumValueObj, nullptr);
		char* utf8Str = mono_string_to_utf8(strObj);
		std::string name(utf8Str);
		mono_free(utf8Str);

		options.push_back(std::make_pair(value, name));
	}

	return options;
}
} // namespace
mono_type::mono_type() = default;

mono_type::mono_type(MonoImage* image, const std::string& name)
	: mono_type(image, "", name)
{
}

mono_type::mono_type(MonoImage* image, const std::string& name_space, const std::string& name)
{
	class_ = mono_class_from_name(image, name_space.c_str(), name.c_str());

	if(!class_)
		throw mono_exception("NATIVE::Could not get class : " + name_space + "." + name);

	generate_meta();
}

mono_type::mono_type(MonoClass* cls)
{
	if(cls)
	{
		class_ = cls;
		generate_meta();
	}
}
mono_type::mono_type(MonoType* type)
{
	if(type)
	{
		class_ = mono_class_from_mono_type(type);
		if(!class_)
			throw mono_exception("NATIVE::Could not get class");

		generate_meta();
	}
}
auto mono_type::valid() const -> bool
{
	return class_ != nullptr;
}

auto mono_type::new_instance() const -> mono_object
{
	const auto& domain = mono_domain::get_current_domain();
	return new_instance(domain);
}

auto mono_type::new_instance(const mono_domain& domain) const -> mono_object
{
	return mono_object(domain, *this);
}

auto mono_type::get_method(const std::string& name_with_args) const -> mono_method
{
	return mono_method(*this, name_with_args);
}

auto mono_type::get_method(const std::string& name, int argc) const -> mono_method
{
	return mono_method(*this, name, argc);
}

auto mono_type::get_field(const std::string& name) const -> mono_field
{
	return mono_field(*this, name);
}

auto mono_type::get_property(const std::string& name) const -> mono_property
{
	return mono_property(*this, name);
}

auto mono_type::get_fields() const -> std::vector<mono_field>
{
	void* iter = nullptr;
	auto field = mono_class_get_fields(class_, &iter);
	std::vector<mono_field> fields;
	while(field)
	{
		std::string name = mono_field_get_name(field);

		fields.emplace_back(get_field(name));

		field = mono_class_get_fields(class_, &iter);
	}
	return fields;
}

auto mono_type::get_properties() const -> std::vector<mono_property>
{
	void* iter = nullptr;
	auto prop = mono_class_get_properties(class_, &iter);
	std::vector<mono_property> props;
	while(prop)
	{
		std::string name = mono_property_get_name(prop);

		props.emplace_back(get_property(name));

		prop = mono_class_get_properties(class_, &iter);
	}
	return props;
}

auto mono_type::get_methods() const -> std::vector<mono_method>
{
	void* iter = nullptr;
	auto method = mono_class_get_methods(class_, &iter);
	std::vector<mono_method> methods;

	while(method != nullptr)
	{
		auto sig = mono_method_signature(method);
		std::string signature = mono_signature_get_desc(sig, false);
		std::string name = mono_method_get_name(method);
		std::string fullname = name + "(" + signature + ")";
		methods.emplace_back(get_method(fullname));
		method = mono_class_get_methods(class_, &iter);
	}

	return methods;
}

auto mono_type::get_attributes() const -> std::vector<mono_object>
{
	std::vector<mono_object> result;

	// Get custom attributes from the class
	MonoCustomAttrInfo* attr_info = mono_custom_attrs_from_class(class_);

	if(attr_info)
	{
		result.reserve(attr_info->num_attrs);
		// Iterate over the custom attributes
		for(int i = 0; i < attr_info->num_attrs; ++i)
		{
			MonoCustomAttrEntry* entry = &attr_info->attrs[i];

			// Get the MonoClass* of the attribute
			MonoClass* attr_class = mono_method_get_class(entry->ctor);

			if(attr_class)
			{
				MonoObject* attr_obj = mono_custom_attrs_get_attr(attr_info, attr_class);
				// Add the attribute instance to the result vector
				if(attr_obj)
				{
					result.emplace_back(attr_obj);
				}
			}
		}
		// Free the attribute info when done
		mono_custom_attrs_free(attr_info);
	}

	return result;
}

auto mono_type::has_base_type() const -> bool
{
	return mono_class_get_parent(class_) != nullptr;
}

auto mono_type::get_base_type() const -> mono_type
{
	auto base = mono_class_get_parent(class_);
	return mono_type(base);
}

auto mono_type::get_nested_types() const -> std::vector<mono_type>
{
	void* iter = nullptr;
	auto nested = mono_class_get_nested_types(class_, &iter);
	std::vector<mono_type> nested_clases;
	while(nested)
	{
		nested_clases.emplace_back(mono_type(nested));
	}
	return nested_clases;
}

auto mono_type::get_internal_ptr() const -> MonoClass*
{
	return class_;
}

void mono_type::generate_meta()
{
#ifndef NDEBUG
	meta_ = std::make_shared<meta_info>();
	meta_->name_space = get_namespace();
	meta_->name = get_name();
	meta_->fullname = get_fullname();
	meta_->rank = get_rank();
	meta_->is_valuetype = is_valuetype();
	meta_->is_enum = is_enum();
	meta_->size = get_sizeof();
	meta_->align = get_alignof();
#endif
}

auto mono_type::is_derived_from(const mono_type& type) const -> bool
{
	return mono_class_is_subclass_of(class_, type.get_internal_ptr(), true) != 0;
}
auto mono_type::get_namespace() const -> std::string
{
	return mono_class_get_namespace(class_);
}
auto mono_type::get_name() const -> std::string
{
	return get_name(false);
}

auto mono_type::get_name(bool full) const -> std::string
{
	MonoType* type = mono_class_get_type(class_);
	if(full)
	{
		return mono_type_get_name(type);
	}

	if(mono_type_get_type(type) != MONO_TYPE_GENERICINST)
	{
		return mono_class_get_name(class_);
	}
	// Get generic arguments as part of the type name
	return strip_namespace(mono_type_get_name(type));
}

auto mono_type::get_fullname() const -> std::string
{
	return get_name(true);
}
auto mono_type::is_valuetype() const -> bool
{
	return !!mono_class_is_valuetype(class_);
}

auto mono_type::mono_type::is_enum() const -> bool
{
	return mono_class_is_enum(class_);
}

auto mono_type::get_enum_base_type() const -> mono_type
{
	return mono_type(mono_class_enum_basetype(class_));
}

template<>
auto mono_type::get_enum_values<uint8_t>() const -> std::vector<std::pair<uint8_t, std::string>>
{
	return get_enum_options<uint8_t>(class_);
}

template<>
auto mono_type::get_enum_values<uint16_t>() const -> std::vector<std::pair<uint16_t, std::string>>
{
	return get_enum_options<uint16_t>(class_);
}

template<>
auto mono_type::get_enum_values<uint32_t>() const -> std::vector<std::pair<uint32_t, std::string>>
{
	return get_enum_options<uint32_t>(class_);
}

template<>
auto mono_type::get_enum_values<uint64_t>() const -> std::vector<std::pair<uint64_t, std::string>>
{
	return get_enum_options<uint64_t>(class_);
}

template<>
auto mono_type::get_enum_values<int8_t>() const -> std::vector<std::pair<int8_t, std::string>>
{
	return get_enum_options<int8_t>(class_);
}

template<>
auto mono_type::get_enum_values<int16_t>() const -> std::vector<std::pair<int16_t, std::string>>
{
	return get_enum_options<int16_t>(class_);
}

template<>
auto mono_type::get_enum_values<int32_t>() const -> std::vector<std::pair<int32_t, std::string>>
{
	return get_enum_options<int32_t>(class_);
}

template<>
auto mono_type::get_enum_values<int64_t>() const -> std::vector<std::pair<int64_t, std::string>>
{
	return get_enum_options<int64_t>(class_);
}

auto mono_type::is_class() const -> bool
{
	return !is_valuetype();
}

auto mono_type::is_struct() const -> bool
{
	return is_valuetype() && !is_enum();
}

auto mono_type::get_rank() const -> int
{
	return mono_class_get_rank(class_);
}

auto mono_type::get_sizeof() const -> uint32_t
{
	uint32_t align{};
	return std::uint32_t(mono_class_value_size(class_, &align));
}

auto mono_type::get_alignof() const -> uint32_t
{
	uint32_t align{};
	mono_class_value_size(class_, &align);
	return align;
}

auto mono_type::is_abstract() const -> bool
{
	return (mono_class_get_flags(class_) & MONO_TYPE_ATTR_ABSTRACT) != 0;
}

auto mono_type::is_sealed() const -> bool
{
	return (mono_class_get_flags(class_) & MONO_TYPE_ATTR_SEALED) != 0;
}

auto mono_type::is_interface() const -> bool
{
	return (mono_class_get_flags(class_) & MONO_TYPE_ATTR_INTERFACE) != 0;
}
} // namespace mono
