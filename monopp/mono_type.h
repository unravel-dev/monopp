#pragma once

#include "mono_config.h"

BEGIN_MONO_INCLUDE
#include "mono/metadata/appdomain.h"
#include <mono/metadata/class.h>
#include <mono/metadata/image.h>
#include <mono/metadata/reflection.h>
END_MONO_INCLUDE

namespace mono
{

class mono_assembly;
class mono_method;
class mono_field;
class mono_property;
class mono_object;
class mono_domain;

class mono_type
{
public:
	mono_type();

	explicit mono_type(MonoImage* image, const std::string& name);
	explicit mono_type(MonoImage* image, const std::string& name_space, const std::string& name);
	explicit mono_type(MonoClass* cls);
	explicit mono_type(MonoType* type);

	auto valid() const -> bool;

	auto new_instance() const -> mono_object;
	auto new_instance(const mono_domain& domain) const -> mono_object;

	auto get_method(const std::string& name_with_args) const -> mono_method;

	auto get_method(const std::string& name, int argc) const -> mono_method;

	auto get_field(const std::string& name) const -> mono_field;

	auto get_property(const std::string& name) const -> mono_property;

	auto get_fields() const -> std::vector<mono_field>;

	auto get_properties() const -> std::vector<mono_property>;

	auto get_methods() const -> std::vector<mono_method>;

	auto get_attributes() const -> std::vector<mono_object>;

	auto has_base_type() const -> bool;

	auto get_base_type() const -> mono_type;

	auto get_nested_types() const -> std::vector<mono_type>;

	auto is_derived_from(const mono_type& type) const -> bool;

	auto get_namespace() const -> std::string;

	auto get_name() const -> std::string;

	auto get_hash() const -> size_t;

	auto get_fullname() const -> std::string;

	auto is_valuetype() const -> bool;

	auto is_struct() const -> bool;

	auto is_class() const -> bool;

	auto is_enum() const -> bool;

	auto get_enum_base_type() const -> mono_type;

	template<typename T>
	auto get_enum_values() const -> std::vector<std::pair<T, std::string>>;

	auto get_rank() const -> int;

	auto get_sizeof() const -> std::uint32_t;

	auto get_alignof() const -> std::uint32_t;

	bool is_abstract() const;
	bool is_sealed() const;
	bool is_interface() const;

	auto get_internal_ptr() const -> MonoClass*;

	static auto get_hash(const std::string& name) -> size_t;
	static auto get_hash(const char* name) -> size_t;

private:
	auto get_name(bool full) const -> std::string;

	void generate_meta();

	non_owning_ptr<MonoClass> class_ = nullptr;

#if MONOPP_DEBUG_LEVEL > 0
	struct meta_info
	{
		std::string name_space;
		std::string name;
		std::string fullname;
		std::uint32_t size = 0;
		std::uint32_t align = 0;
		int rank = 0;
		bool is_valuetype = true;
		bool is_enum = false;
	};
	std::shared_ptr<meta_info> meta_{};
#endif
};

} // namespace mono
