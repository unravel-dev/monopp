#include "managed_object.h"
#include <monopp/mono_internal_call.h>
#include <monopp/mono_jit.h>
#include <monopp/mono_type.h>

namespace mono
{
namespace managed_interface
{

void finalize(const mono::mono_object& obj)
{
	// get the cpp pointer associated with the mono object
	auto this_cpp_ptr = &object::get_managed_object_as<object>(obj);

	// invalidate the mono object's handle to the cpp pointer
	const auto& field = *object::get_native_object_field();
	auto mutable_field = make_field_invoker<object*>(field);
	mutable_field.set_value(obj, nullptr);
	// delete the cpp pointer
	delete this_cpp_ptr;
}
void object::register_internal_calls()
{
	add_internal_call("Monopp.Core.NativeObject::Finalize", internal_call(finalize));
}

void object::initialize_type_field(const mono_assembly& assembly)
{
	auto type = assembly.get_type("Monopp.Core", "NativeObject");
	auto& object_type = get_object_type();
	object_type = std::make_unique<mono_type>(std::move(type));
	auto field = object_type->get_field("native_this_");
	auto& native_object_field = get_native_object_field();
	native_object_field = std::make_unique<mono_field>(std::move(field));
}

object::~object() = default;

object::object(mono_object obj)
	: gc_scoped_handle_(obj)
{
	auto pinned = gc_scoped_handle_.get_object();
	assert(pinned.get_type().is_derived_from(*get_object_type()) &&
		   "Mono wrapper classes must inherit from Monopp.Core.NativeObject.");

	// Give mono the ownership of the this pointer.
	// When the c# finalize is called then our finalize will
	// delete the pointer
	const auto& field = *object::get_native_object_field();
	auto mutable_field = make_field_invoker<object*>(field);
	mutable_field.set_value(pinned, this);
}

auto object::get_object_type() -> std::unique_ptr<mono_type>&
{
	static std::unique_ptr<mono_type> object_type;
	return object_type;
}

auto object::get_native_object_field() -> std::unique_ptr<mono_field>&
{
	static std::unique_ptr<mono_field> native_object_field;
	return native_object_field;
}

} // namespace managed_interface
} // namespace mono
