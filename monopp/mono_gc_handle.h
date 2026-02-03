#pragma once

#include "mono_config.h"

#include "mono_object.h"
#include "mono_array.h"
#include "mono_list.h"

namespace mono
{


class mono_scoped_gc_handle
{
public:
	mono_scoped_gc_handle() = default;
	mono_scoped_gc_handle(const mono_scoped_gc_handle&) noexcept = delete;
	auto operator=(const mono_scoped_gc_handle&) noexcept -> mono_scoped_gc_handle& = delete;

	explicit mono_scoped_gc_handle(const mono_object& obj)
	{
		lock(obj);
	}

	~mono_scoped_gc_handle()
	{
		unlock();
	}

	
	void lock(const mono_object& obj);
	void unlock();
	
	auto is_locked() const -> bool
	{
		return handle_ != 0;
	}

	auto get_handle() const -> uint32_t
	{
		return handle_;
	}

	auto get_domain_version() const -> intptr_t
	{
		return domain_version_;
	}

	auto get_object() const -> mono_object;

	/// <summary>
	/// Get the object as a specific type
	/// </summary>
	template<typename T>
	auto get_object_as() const -> T
	{
		return T(get_object());
	}

private:
	uint32_t handle_ = 0;
	intptr_t domain_version_ = 0;
};

using mono_object_pinned = mono_scoped_gc_handle;
using mono_object_pinned_ptr = std::shared_ptr<mono_object_pinned>;

inline auto make_object_pinned(const mono_object& obj) -> mono_object_pinned_ptr
{
	return std::make_shared<mono_object_pinned>(obj);
}


template<typename T>
struct mono_array_pinned : mono_object_pinned
{
	using mono_object_pinned::mono_object_pinned;
	/// <summary>
	/// Get access to the array (read-only, handle remains locked)
	/// </summary>
	auto get_array() const -> mono_array<T>
	{
		return get_object_as<mono_array<T>>();
	}

};

template<typename T>
using mono_array_pinned_ptr = std::shared_ptr<mono_array_pinned<T>>;

template<typename T>
inline auto make_array_pinned(const mono_array<T>& obj) -> mono_array_pinned_ptr<T>
{
	return std::make_shared<mono_array_pinned<T>>(obj);
}


template<typename T>
struct mono_list_pinned : mono_object_pinned
{
	using mono_object_pinned::mono_object_pinned;
	/// <summary>
	/// Get access to the list (read-only, handle remains locked)
	/// </summary>
	auto get_list() const -> mono_list<T>
	{
		return get_object_as<mono_list<T>>();
	}

};

template<typename T>
using mono_list_pinned_ptr = std::shared_ptr<mono_list_pinned<T>>;

template<typename T>
inline auto make_list_pinned(const mono_list<T>& obj) -> mono_list_pinned_ptr<T>
{
	return std::make_shared<mono_list_pinned<T>>(obj);
}

template<class Fn>
auto with_pinned(const mono_object& obj, Fn&& fn) -> decltype(fn(mono_object()))
{
  mono_scoped_gc_handle pinned(obj);
  return fn(pinned.get_object());
}

auto gc_get_heap_size() -> int64_t;
auto gc_get_used_size() -> int64_t;
void gc_collect();


} // namespace mono
