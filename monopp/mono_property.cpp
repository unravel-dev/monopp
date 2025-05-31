#include "mono_property.h"
#include "mono_assembly.h"
#include "mono_exception.h"
#include "mono_method.h"
#include "mono_object.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/appdomain.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/debug-helpers.h>
END_MONO_INCLUDE

namespace mono
{

mono_property::mono_property(const mono_type& type, const std::string& name)
	: property_(mono_class_get_property_from_name(type.get_internal_ptr(), name.c_str()))
{
	if(!property_)
		throw mono_exception("NATIVE::Could not get property : " + name + " for class " + type.get_name());

	auto get_method = get_get_method();
	type_ = get_method.get_return_type();

	generate_meta();
}

auto mono_property::get_internal_ptr() const -> MonoProperty*
{
	return property_;
}

auto mono_property::get_name() const -> std::string
{
	return mono_property_get_name(get_internal_ptr());
}

auto mono_property::get_fullname() const -> std::string
{
	return mono_property_get_name(get_internal_ptr());
}

auto mono_property::get_full_declname() const -> std::string
{
	std::string storage = (is_static() ? " static " : " ");
	return to_string(get_visibility()) + storage + get_name();
}

auto mono_property::get_type() const -> const mono_type&
{
	return type_;
}

auto mono_property::get_get_method() const -> mono_method
{
	auto method = mono_property_get_get_method(property_);
	return mono_method(method);
}
auto mono_property::get_set_method() const -> mono_method
{
	auto method = mono_property_get_set_method(property_);
	return mono_method(method);
}

auto mono_property::get_visibility() const -> visibility
{
	auto getter_vis = visibility::vis_public;
	try
	{
		auto getter = get_get_method();
		getter_vis = getter.get_visibility();
	}
	catch(const mono_exception&)
	{
	}
	auto setter_vis = visibility::vis_public;
	try
	{
		auto setter = get_set_method();
		if(setter)
		{
			setter_vis = setter.get_visibility();
		}
	}
	catch(const mono_exception&)
	{
	}
	if(int(getter_vis) < int(setter_vis))
	{
		return getter_vis;
	}

	return setter_vis;
}

auto mono_property::is_static() const -> bool
{
	auto getter = get_get_method();
	return getter.is_static();
}

auto mono_property::is_readonly() const -> bool
{
	auto getter = get_set_method();
	return !getter.valid();
}


void mono_property::generate_meta()
{
#if MONOPP_DEBUG_LEVEL > 0
	meta_ = std::make_shared<meta_info>();
	meta_->name = get_name();
	meta_->fullname = get_fullname();
	meta_->full_declname = get_full_declname();
#endif
}

auto mono_property::get_attributes() const -> std::vector<mono_object>
{
	std::vector<mono_object> result;

	auto parent_class = mono_property_get_parent(property_);

	// Get custom attributes from the property
	MonoCustomAttrInfo* attr_info = mono_custom_attrs_from_property(parent_class, property_);

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

auto mono_property::is_special_name() const -> bool
{
	uint32_t flags = mono_property_get_flags(property_);
	return (flags & MONO_PROPERTY_ATTR_SPECIAL_NAME) != 0;
}

auto mono_property::has_default() const -> bool
{
	uint32_t flags = mono_property_get_flags(property_);
	return (flags & MONO_PROPERTY_ATTR_HAS_DEFAULT) != 0;
}
} // namespace mono
