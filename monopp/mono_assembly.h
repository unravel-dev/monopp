#pragma once

#include "mono_config.h"
#include "mono_type.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/image.h>
END_MONO_INCLUDE

namespace mono
{

class mono_domain;

class mono_assembly
{
public:
	explicit mono_assembly(const mono_domain& domain, const std::string& path, bool shared = true);
	explicit mono_assembly(MonoImage* image);

	auto get_type(const std::string& name) const -> mono_type;
	auto get_type(const std::string& name_space, const std::string& name) const -> mono_type;

	auto get_types() const -> std::vector<mono_type>;
	auto get_types_derived_from(const mono_type& base) const -> std::vector<mono_type>;


	static auto get_corlib() -> mono_assembly;
	auto dump_references() const -> std::vector<std::string>;

private:
	non_owning_ptr<MonoAssembly> assembly_ = nullptr;
	non_owning_ptr<MonoImage> image_ = nullptr;
};

} // namespace mono
