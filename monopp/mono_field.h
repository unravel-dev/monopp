#pragma once

#include "mono_config.h"

#include "mono_type.h"
#include "mono_visibility.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/object.h>
END_MONO_INCLUDE

namespace mono
{
class mono_object;

class mono_field
{
public:
	explicit mono_field(const mono_type& type, const std::string& name);

	auto get_name() const -> std::string;

	auto get_fullname() const -> std::string;

	auto get_full_declname() const -> std::string;

	auto get_type() const -> const mono_type&;

	auto get_visibility() const -> visibility;

	auto is_static() const -> bool;

	auto get_attributes() const -> std::vector<mono_type>;

	auto is_readonly() const -> bool;

	auto is_const() const -> bool;

protected:
	void generate_meta();

	auto is_valuetype() const -> bool;

	mono_type type_;

	non_owning_ptr<MonoClassField> field_ = nullptr;

	non_owning_ptr<MonoVTable> owning_type_vtable_ = nullptr;

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
