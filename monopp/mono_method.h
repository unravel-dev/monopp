#pragma once

#include "mono_config.h"

#include "mono_type.h"
#include "mono_visibility.h"

namespace mono
{

class mono_method
{
public:
	explicit mono_method(MonoMethod* method);
	explicit mono_method(const mono_type& type, const std::string& name_with_args);
	explicit mono_method(const mono_type& type, const std::string& name, int argc);

	auto get_return_type() const -> mono_type;

	auto get_param_types() const -> std::vector<mono_type>;

	auto get_name() const -> std::string;

	auto get_fullname() const -> std::string;

	auto get_full_declname() const -> std::string;

	auto get_visibility() const -> visibility;

	auto is_static() const -> bool;

	auto is_virtual() const -> bool;

	auto is_pinvoke_impl() const -> bool;

	auto is_special_name() const -> bool;

	auto is_internal_call() const -> bool;

	auto is_synchronized() const -> bool;

	auto get_attributes() const -> std::vector<mono_type>;

	auto valid() const -> bool;
	operator bool() const;

	auto get_internal_ptr() const -> MonoMethod*;

protected:
	void generate_meta();

	non_owning_ptr<MonoMethod> method_ = nullptr;
	non_owning_ptr<MonoMethodSignature> signature_ = nullptr;


#ifndef NDEBUG
	struct meta_info
	{
		std::string name;
		std::string fullname;
		std::string full_declname;
	};

	std::shared_ptr<meta_info> meta_{};
#endif

};

} // namespace mono
