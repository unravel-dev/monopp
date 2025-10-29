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
	struct meta_info;

	explicit mono_field(const mono_type& type, const std::string& name);

	auto get_name() const -> std::string;

	auto get_fullname() const -> std::string;

	auto get_full_declname() const -> std::string;

	auto get_type() const -> const mono_type&;

	auto get_visibility() const -> visibility;

	auto is_static() const -> bool;

	auto get_attributes() const -> std::vector<mono_object>;

	auto has_attribute(const std::string& attribute_full_name) const -> bool;

	auto is_readonly() const -> bool;

	auto is_const() const -> bool;

	auto is_backing_field() const -> bool;

protected:
	void generate_meta();

	auto is_valuetype() const -> bool;

	mono_type type_;

	non_owning_ptr<MonoClassField> field_ = nullptr;

	non_owning_ptr<MonoVTable> owning_type_vtable_ = nullptr;

	std::shared_ptr<meta_info> meta_{};
};

void reset_field_cache();

} // namespace mono
