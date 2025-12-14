#include "mono_gc_handle.h"

BEGIN_MONO_INCLUDE
#include <mono/metadata/mono-gc.h>
END_MONO_INCLUDE

namespace mono
{

void mono_gc_handle::lock()
{
	assert(handle_ == 0);
	if(object_)
	{
		handle_ = mono_gchandle_new(object_, 1);
	}
}

void mono_gc_handle::unlock()
{
	assert(handle_ != 0);
	mono_gchandle_free(handle_);
	handle_ = 0;
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
