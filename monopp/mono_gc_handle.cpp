#include "mono_gc_handle.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/mono-gc.h>
END_MONO_INCLUDE

namespace mono
{

void mono_scoped_gc_handle::lock(const mono_object& obj)
{
	assert(handle_ == 0);
	if(obj.valid())
	{
		handle_ = mono_gchandle_new(obj.get_internal_ptr(), 1);
	}
}

void mono_scoped_gc_handle::unlock()
{
	if(handle_ != 0)
	{
		mono_gchandle_free(handle_);

	}
	handle_ = 0;
}

auto mono_scoped_gc_handle::get_object() const -> mono_object
{
	if(handle_ == 0)
	{
		return mono_object();
	}
	return mono_object(mono_gchandle_get_target(handle_));
}

auto gc_get_heap_size() -> int64_t
{
	return mono_gc_get_heap_size();
}
auto gc_get_used_size() -> int64_t
{
	return mono_gc_get_used_size();
}

void gc_collect()
{
	mono_gc_collect(mono_gc_max_generation());
}

} // namespace mono
