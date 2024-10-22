#include "mono_method.h"
#include "mono_exception.h"
#include "mono_type.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/appdomain.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/debug-helpers.h>
END_MONO_INCLUDE

namespace mono
{

mono_method::mono_method(MonoMethod* method)
{
	method_ = method;
	if(!method_)
	{
		throw mono_exception("NATIVE::Could not create method");
	}
	generate_meta();
}

mono_method::mono_method(const mono_type& type, const std::string& name_with_args)
{
	auto desc = mono_method_desc_new((":" + name_with_args).c_str(), 0);
	method_ = mono_method_desc_search_in_class(desc, type.get_internal_ptr());
	mono_method_desc_free(desc);

	if(!method_)
	{
		const auto& type_name = type.get_name();
		throw mono_exception("NATIVE::Could not create method : " + name_with_args + " for class " +
							 type_name);
	}
	generate_meta();
}

mono_method::mono_method(const mono_type& type, const std::string& name, int argc)
{
	method_ = mono_class_get_method_from_name(type.get_internal_ptr(), name.c_str(), argc);

	if(!method_)
	{
		const auto& type_name = type.get_name();
		throw mono_exception("NATIVE::Could not create method : " + name + " for class " + type_name);
	}
	generate_meta();
}

void mono_method::generate_meta()
{
	signature_ = mono_method_signature(method_);
	name_ = mono_method_get_name(method_);
	fullname_ = mono_method_full_name(method_, true);
	std::string storage = (is_static() ? " static " : " ");
	full_declname_ = to_string(get_visibility()) + storage + fullname_;
}

auto mono_method::get_return_type() const -> mono_type
{
	auto type = mono_signature_get_return_type(signature_);
	return mono_type(type);
}

auto mono_method::get_param_types() const -> std::vector<mono_type>
{
	void* iter = nullptr;
	auto type = mono_signature_get_params(signature_, &iter);
	std::vector<mono_type> params;
	while(type)
	{
		params.emplace_back(type);

		type = mono_signature_get_params(signature_, &iter);
	}

	return params;
}

auto mono_method::get_name() const -> const std::string&
{
	return name_;
}

auto mono_method::get_fullname() const -> const std::string&
{
	return fullname_;
}
auto mono_method::get_full_declname() const -> const std::string&
{
	return full_declname_;
}
auto mono_method::get_visibility() const -> visibility
{

	uint32_t flags = mono_method_get_flags(method_, nullptr) & MONO_METHOD_ATTR_ACCESS_MASK;

	if(flags == MONO_METHOD_ATTR_PRIVATE)
		return visibility::vis_private;
	else if(flags == MONO_METHOD_ATTR_FAM_AND_ASSEM)
		return visibility::vis_protected_internal;
	else if(flags == MONO_METHOD_ATTR_ASSEM)
		return visibility::vis_internal;
	else if(flags == MONO_METHOD_ATTR_FAMILY)
		return visibility::vis_protected;
	else if(flags == MONO_METHOD_ATTR_PUBLIC)
		return visibility::vis_public;

	assert(false);

	return visibility::vis_private;
}

auto mono_method::is_static() const -> bool
{
	uint32_t flags = mono_method_get_flags(method_, nullptr);
	return (flags & MONO_METHOD_ATTR_STATIC) != 0;
}
auto mono_method::is_virtual() const -> bool
{
	uint32_t flags = mono_method_get_flags(method_, nullptr);
	return (flags & MONO_METHOD_ATTR_VIRTUAL) != 0;
}

auto mono_method::is_pinvoke_impl() const -> bool
{
	uint32_t flags = mono_method_get_flags(method_, nullptr);
	return (flags & MONO_METHOD_ATTR_PINVOKE_IMPL) != 0;
}

auto mono_method::is_special_name() const -> bool
{
	uint32_t flags = mono_method_get_flags(method_, nullptr);
	return (flags & MONO_METHOD_ATTR_SPECIAL_NAME) != 0;
}

auto mono_method::is_internal_call() const -> bool
{
	uint32_t impl_flags = 0;
	mono_method_get_flags(method_, &impl_flags);
	return (impl_flags & MONO_METHOD_IMPL_ATTR_INTERNAL_CALL) != 0;
}

auto mono_method::is_synchronized() const -> bool
{
	uint32_t impl_flags = 0;
	mono_method_get_flags(method_, &impl_flags);
	return (impl_flags & MONO_METHOD_IMPL_ATTR_SYNCHRONIZED) != 0;
}

auto mono_method::get_attributes() const -> std::vector<mono_type>
{
	std::vector<mono_type> result;

	// Get custom attributes from the method
	MonoCustomAttrInfo* attr_info = mono_custom_attrs_from_method(method_);

	if(attr_info)
	{
		result.reserve(attr_info->num_attrs);

		// Iterate over the custom attributes
		for(int i = 0; i < attr_info->num_attrs; ++i)
		{
			MonoCustomAttrEntry* entry = &attr_info->attrs[i];

			// Get the MonoClass* of the attribute
			MonoClass* attr_class = mono_method_get_class(entry->ctor);

			result.emplace_back(attr_class);
		}

		// Free the attribute info when done
		mono_custom_attrs_free(attr_info);
	}

	// Get method flags
	uint32_t impl_flags{};
	uint32_t flags = mono_method_get_flags(method_, &impl_flags);

	MonoImage* corlib = mono_get_corlib(); // Get corlib once for efficiency

	// Check for SpecialName
	if((flags & MONO_METHOD_ATTR_SPECIAL_NAME) != 0)
	{
		MonoClass* special_name_attr_class =
			mono_class_from_name(corlib, "System.Runtime.CompilerServices", "SpecialNameAttribute");
		if(special_name_attr_class)
		{
			result.emplace_back(special_name_attr_class);
		}
	}

	// Check for PInvokeImpl
	if((flags & MONO_METHOD_ATTR_PINVOKE_IMPL) != 0)
	{
		MonoClass* dll_import_attr_class =
			mono_class_from_name(corlib, "System.Runtime.InteropServices", "DllImportAttribute");
		if(dll_import_attr_class)
		{
			result.emplace_back(dll_import_attr_class);
		}
	}

	// Check for MethodImpl attributes

	if((impl_flags & MONO_METHOD_IMPL_ATTR_INTERNAL_CALL) != 0)
	{
		// InternalCall methods may correspond to [MethodImpl(MethodImplOptions.InternalCall)]
		// There is no direct attribute class, but you can note this flag
	}

	if((impl_flags & MONO_METHOD_IMPL_ATTR_SYNCHRONIZED) != 0)
	{
		// Synchronized methods correspond to [MethodImpl(MethodImplOptions.Synchronized)]
		MonoClass* method_impl_attr_class =
			mono_class_from_name(corlib, "System.Runtime.CompilerServices", "MethodImplAttribute");
		if(method_impl_attr_class)
		{
			result.emplace_back(method_impl_attr_class);
		}
	}

	// Add more checks for other MethodImplOptions as needed

	return result;
}
} // namespace mono
