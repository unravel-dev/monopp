#include "mono_string.h"
#include "mono_domain.h"
#include "utf8/unchecked.h"
namespace mono
{

mono_string::mono_string(const mono_object& obj)
	: mono_object(obj)
{
}

mono_string::mono_string(const mono_domain& domain, const std::string& str)
	: mono_object(reinterpret_cast<MonoObject*>(mono_string_new(domain.get_internal_ptr(), str.c_str())))
{
}

auto mono_string::as_utf8() const -> std::string
{
	// TODO: This could be probably optimized by doing no additional
	// allocation though mono_string_chars and mono_string_length.
	// MonoString* mono_str = mono_object_to_string(get_internal_ptr(), nullptr);

	// auto raw_str = mono_string_to_utf8(mono_str);
	// std::string str = reinterpret_cast<std::string::value_type*>(raw_str);
	// mono_free(raw_str);
	// return str;

	// Convert any object to a MonoString:
	MonoString* mono_str = mono_object_to_string(get_internal_ptr(), nullptr);

	// Retrieve internal UTF-16 buffer & length:
	mono_unichar2* utf16_data = mono_string_chars(mono_str);
	int utf16_len = mono_string_length(mono_str);
	if(utf16_len <= 0 || !utf16_data)
	{
		return {};
	}

	// Reserve space in the output UTF-8 string:
	std::string utf8;
	utf8.reserve(static_cast<size_t>(utf16_len) * 3 + 1);

	// Transcode using the unchecked library:
	utf8::unchecked::utf16to8(reinterpret_cast<const utf8::utfchar16_t*>(utf16_data),
							  reinterpret_cast<const utf8::utfchar16_t*>(utf16_data + utf16_len),
							  std::back_inserter(utf8));

	return utf8;
}

auto mono_string::as_utf16() const -> std::u16string
{
	// TODO: This could be probably optimized by doing no additional
	// allocation though mono_string_chars and mono_string_length.
	MonoString* mono_str = mono_object_to_string(get_internal_ptr(), nullptr);

	auto raw_str = mono_string_to_utf16(mono_str);
	std::u16string str = reinterpret_cast<std::u16string::value_type*>(raw_str);
	mono_free(raw_str);
	return str;
}

auto mono_string::as_utf32() const -> std::u32string
{
	// TODO: This could be probably optimized by doing no additional
	// allocation though mono_string_chars and mono_string_length.
	MonoString* mono_str = mono_object_to_string(get_internal_ptr(), nullptr);

	auto raw_str = mono_string_to_utf32(mono_str);
	std::u32string str = reinterpret_cast<std::u32string::value_type*>(raw_str);
	mono_free(raw_str);
	return str;
}

} // namespace mono
