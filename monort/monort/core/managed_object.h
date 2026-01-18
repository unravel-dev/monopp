#pragma once

#include <monopp/mono_assembly.h>
#include <monopp/mono_field_invoker.h>
#include <monopp/mono_gc_handle.h>
#include <monopp/mono_object.h>
#include <monopp/mono_type.h>
#include <memory>

namespace mono
{
namespace managed_interface
{

class object
{
public:
	static void register_internal_calls();
	static void initialize_type_field(const mono_assembly& assembly);

	explicit object(mono_object obj);
	virtual ~object();

	object(const object&) noexcept = delete;
	auto operator=(const object&) noexcept -> object& = delete;

	static auto get_object_type() -> std::unique_ptr<mono_type>&;
	static auto get_native_object_field() -> std::unique_ptr<mono_field>&;

	template <typename T>
	static auto get_managed_object_as(const mono_object& mono_obj) -> auto&;

private:
	mono_scoped_gc_handle gc_scoped_handle_;
};

template <typename T>
auto object::get_managed_object_as(const mono_object& mono_obj) -> auto&
{
	const auto& field = *get_native_object_field();
	auto mutable_field = make_field_invoker<T*>(field);
	return *mutable_field.get_value(mono_obj);
}

} // namespace managed_interface
} // namespace mono
