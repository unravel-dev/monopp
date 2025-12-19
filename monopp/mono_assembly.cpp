#include "mono_assembly.h"
#include "mono_domain.h"
#include "mono_exception.h"

#include "mono_string.h"
#include "mono_type.h"
#include <fstream>
#include <sstream>

BEGIN_MONO_INCLUDE
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/image.h>
#include <mono/metadata/tokentype.h>
END_MONO_INCLUDE

namespace mono
{

namespace
{
template <typename Container = std::string, typename CharT = char, typename Traits = std::char_traits<char>>
auto read_stream_into_container(std::basic_istream<CharT, Traits>& in, Container& container) -> bool
{
	static_assert(
		std::is_same<Container,
					 std::basic_string<CharT, Traits, typename Container::allocator_type>>::value ||
			std::is_same<Container, std::vector<CharT, typename Container::allocator_type>>::value ||
			std::is_same<Container, std::vector<std::make_unsigned_t<CharT>,
												typename Container::allocator_type>>::value ||
			std::is_same<Container,
						 std::vector<std::make_signed_t<CharT>, typename Container::allocator_type>>::value,
		"only strings and vectors of ((un)signed) CharT allowed");

	auto const start_pos = in.tellg();
	if(std::streamsize(-1) == start_pos || !in.good())
	{
		return false;
	}

	if(!in.seekg(0, std::ios_base::end) || !in.good())
	{
		return false;
	}

	auto const end_pos = in.tellg();

	if(std::streamsize(-1) == end_pos || !in.good())
	{
		return false;
	}

	auto const char_count = end_pos - start_pos;

	if(!in.seekg(start_pos) || !in.good())
	{
		return false;
	}

	container.resize(static_cast<std::size_t>(char_count));

	if(!container.empty())
	{
		in.read(reinterpret_cast<CharT*>(&container[0]), char_count);
		container.resize(in.gcount());
	}

	return in.good() || in.eof();
}
} // namespace

mono_assembly::mono_assembly(MonoImage* image)
	: image_(image)
{
}

mono_assembly::mono_assembly(const mono_domain& domain, const std::string& path, bool shared)
{
	if(shared)
	{
		assembly_ = mono_domain_assembly_open(domain.get_internal_ptr(), path.c_str());

		if(!assembly_)
			throw mono_exception("NATIVE::Could not open assembly with path : " + path);

		image_ = mono_assembly_get_image(assembly_);
	}
	else
	{
		mono_domain_set(domain.get_internal_ptr(), true);

		// Read the file into memory
		std::ifstream file(path, std::ios::binary); // | std::ios::ate);
		if(!file.is_open())
		{
			throw mono_exception("NATIVE::Could not open assembly with path : " + path);
		}
		std::vector<char> buffer;
		if(!read_stream_into_container(file, buffer))
		{
			throw mono_exception("NATIVE::Could not read assembly with path : " + path);
		}

		// Load the assembly from memory
		MonoImageOpenStatus status;
		image_ =
			mono_image_open_from_data(buffer.data(), static_cast<uint32_t>(buffer.size()), true, &status);
		if(!image_ || status != MONO_IMAGE_OK)
		{
			throw mono_exception("NATIVE::Failed to load assembly from memory with path : " + path);
			return;
		}

		assembly_ = mono_assembly_load_from(image_, path.c_str(), &status);

		if(!assembly_)
			throw mono_exception("NATIVE::Could not open assembly with path : " + path);
	}
}

auto mono_assembly::get_type(const std::string& name) const -> mono_type
{
	return mono_type(image_, name);
}

auto mono_assembly::get_type(const std::string& name_space, const std::string& name) const -> mono_type
{
	return mono_type(image_, name_space, name);
}

auto mono_assembly::get_corlib() -> mono_assembly
{
	return mono_assembly(mono_get_corlib());
}

std::vector<mono_type> mono_assembly::get_types() const
{
	// Get all types in an assembly
	std::vector<mono_type> classes;

	int num_types = mono_image_get_table_rows(image_, MONO_TABLE_TYPEDEF);

	for(int i = 1; i <= num_types; ++i)
	{
		MonoClass* klass = mono_class_get(image_, (i + MONO_TOKEN_TYPE_DEF));
		if(klass)
		{
			classes.push_back(mono_type(klass));
		}
	}
	return classes;
}

auto mono_assembly::get_types_derived_from(const mono_type& base) const -> std::vector<mono_type>
{
	auto types = get_types();

	std::vector<mono_type> result;

	for(const auto& type : types)
	{
		if(type.is_derived_from(base))
		{
			result.emplace_back(type);
		}
	}

	return result;
}

auto mono_assembly::dump_references() const -> std::vector<std::string>
{
	std::vector<std::string> refs;
	/* Get a pointer to the AssemblyRef metadata table */
	auto table_info = mono_image_get_table_info(image_, MONO_TABLE_ASSEMBLYREF);

	/* Fetch the number of rows available in the table */
	int rows = mono_table_info_get_rows(table_info);

	refs.reserve(size_t(rows));
	/* For each row, print some of its values */
	for(int i = 0; i < rows; i++)
	{
		/* Space where we extract one row from the metadata table */
		uint32_t cols[MONO_ASSEMBLYREF_SIZE];

		/* Extract the row into the array cols */
		mono_metadata_decode_row(table_info, i, cols, MONO_ASSEMBLYREF_SIZE);

		std::stringstream s;
		s << i + 1;
		s << " Version=";
		s << cols[MONO_ASSEMBLYREF_MAJOR_VERSION];
		s << ".";
		s << cols[MONO_ASSEMBLYREF_MINOR_VERSION];
		s << ".";
		s << cols[MONO_ASSEMBLYREF_BUILD_NUMBER];
		s << ".";
		s << cols[MONO_ASSEMBLYREF_REV_NUMBER];
		s << "\n\tName=";
		s << mono_metadata_string_heap(image_, cols[MONO_ASSEMBLYREF_NAME]);

		refs.emplace_back(s.str());
	}

	return refs;
}

} // namespace mono
