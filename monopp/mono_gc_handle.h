#pragma once

#include "mono_config.h"

#include "mono_object.h"
#include "mono_array.h"
#include "mono_list.h"

namespace mono
{

class mono_gc_handle : public mono_object
{
public:
	using mono_object::mono_object;

	void lock();
	void unlock();
	
	auto is_locked() const -> bool
	{
		return handle_ != 0;
	}

private:
	std::uint32_t handle_ = 0;
};

class mono_scoped_gc_handle
{
public:
	mono_scoped_gc_handle(const mono_scoped_gc_handle&) noexcept = delete;
	auto operator=(const mono_scoped_gc_handle&) noexcept -> mono_scoped_gc_handle& = delete;

	explicit mono_scoped_gc_handle(mono_gc_handle& handle)
		: handle_(handle)
	{
		handle_.lock();
	}

	~mono_scoped_gc_handle()
	{
		handle_.unlock();
	}

	auto get_handle() const -> mono_gc_handle&
	{
		return handle_;
	}

private:
	mono_gc_handle& handle_;
};

/// <summary>
/// RAII proxy for mutating a pinned object while temporarily unlocking the GC handle
/// Works with mono_object, mono_array<T>, mono_list<T>, and any type with get_internal_ptr()
/// </summary>
template<typename ObjectType>
class mono_pin_proxy
{
public:
	explicit mono_pin_proxy(ObjectType& obj, mono_gc_handle& handle)
		: object_(obj)
		, handle_(handle)
		, was_locked_(handle.is_locked())
	{
		if(was_locked_)
		{
			handle_.unlock();
		}
	}

	~mono_pin_proxy()
	{
		// Update the handle's internal pointer to match the object's current pointer
		// in case set_value() was called during mutation
		handle_.set_value(object_.get_internal_ptr());
		
		if(was_locked_)
		{
			handle_.lock();
		}
	}

	mono_pin_proxy(const mono_pin_proxy&) = delete;
	auto operator=(const mono_pin_proxy&) = delete;
	
	// Move constructor - needed for return by value, but proxy should be used immediately
	mono_pin_proxy(mono_pin_proxy&& other) noexcept
		: object_(other.object_)
		, handle_(other.handle_)
		, was_locked_(other.was_locked_)
	{
		// Transfer ownership - prevent destructor from unlocking
		other.was_locked_ = false;
	}
	
	auto operator=(mono_pin_proxy&&) = delete;

	/// <summary>
	/// Access the object for mutation
	/// </summary>
	auto operator->() -> ObjectType*
	{
		return &object_;
	}

	/// <summary>
	/// Access the object for mutation (const)
	/// </summary>
	auto operator->() const -> const ObjectType*
	{
		return &object_;
	}

	/// <summary>
	/// Dereference to get object reference
	/// </summary>
	auto operator*() -> ObjectType&
	{
		return object_;
	}

	/// <summary>
	/// Dereference to get object reference (const)
	/// </summary>
	auto operator*() const -> const ObjectType&
	{
		return object_;
	}

private:
	ObjectType& object_;
	mono_gc_handle& handle_;
	bool was_locked_;
};

struct mono_object_pinned
{

	explicit mono_object_pinned(const mono_object& obj)
		: object(obj.get_internal_ptr())
		, lock(object)
	{}
	~mono_object_pinned() = default;

	mono_object_pinned(const mono_object_pinned&) noexcept = delete;
	auto operator=(const mono_object_pinned&) noexcept -> mono_object_pinned& = delete;

	/// <summary>
	/// Get a proxy for mutating the object while temporarily unlocking the GC handle
	/// The handle will be re-locked when the proxy is destroyed
	/// </summary>
	auto mutate_pinned() -> mono_pin_proxy<mono_object>
	{
		return mono_pin_proxy<mono_object>(object, object);
	}

	/// <summary>
	/// Get access to the object (read-only, handle remains locked)
	/// </summary>
	auto get_object() const -> const mono_object&
	{
		return object;
	}

	mono_gc_handle object;
	mono_scoped_gc_handle lock;
};

using mono_object_pinned_ptr = std::shared_ptr<mono_object_pinned>;

inline auto make_object_pinned(const mono_object& obj) -> mono_object_pinned_ptr
{
	return std::make_shared<mono_object_pinned>(obj);
}

template<typename T>
struct mono_array_pinned
{

	explicit mono_array_pinned(const mono_array<T>& obj)
		: array(obj)
		, object(array.get_internal_ptr())
		, lock(object)
	{}
	~mono_array_pinned() = default;

	mono_array_pinned(const mono_array_pinned&) noexcept = delete;
	auto operator=(const mono_array_pinned&) noexcept -> mono_array_pinned& = delete;

	/// <summary>
	/// Get a proxy for mutating the array while temporarily unlocking the GC handle
	/// The handle will be re-locked when the proxy is destroyed
	/// </summary>
	auto mutate_pinned() -> mono_pin_proxy<mono_array<T>>
	{
		return mono_pin_proxy<mono_array<T>>(array, object);
	}

	/// <summary>
	/// Get access to the array (read-only, handle remains locked)
	/// </summary>
	auto get_array() const -> const mono_array<T>&
	{
		return array;
	}

	mono_array<T> array;
	mono_gc_handle object;
	mono_scoped_gc_handle lock;
};

template<typename T>
using mono_array_pinned_ptr = std::shared_ptr<mono_array_pinned<T>>;

template<typename T>
inline auto make_array_pinned(const mono_array<T>& obj) -> mono_array_pinned_ptr<T>
{
	return std::make_shared<mono_array_pinned<T>>(obj);
}


template<typename T>
struct mono_list_pinned
{

	explicit mono_list_pinned(const mono_list<T>& obj)
		: list(obj)
		, object(list.get_internal_ptr())
		, lock(object)
	{}
	~mono_list_pinned() = default;

	mono_list_pinned(const mono_list_pinned&) noexcept = delete;
	auto operator=(const mono_list_pinned&) noexcept -> mono_list_pinned& = delete;

	/// <summary>
	/// Get a proxy for mutating the list while temporarily unlocking the GC handle
	/// The handle will be re-locked when the proxy is destroyed
	/// </summary>
	auto mutate_pinned() -> mono_pin_proxy<mono_list<T>>
	{
		return mono_pin_proxy<mono_list<T>>(list, object);
	}

	/// <summary>
	/// Get access to the list (read-only, handle remains locked)
	/// </summary>
	auto get_list() const -> const mono_list<T>&
	{
		return list;
	}

	mono_list<T> list;
	mono_gc_handle object;
	mono_scoped_gc_handle lock;
};

template<typename T>
using mono_list_pinned_ptr = std::shared_ptr<mono_list_pinned<T>>;

template<typename T>
inline auto make_list_pinned(const mono_list<T>& obj) -> mono_list_pinned_ptr<T>
{
	return std::make_shared<mono_list_pinned<T>>(obj);
}

auto gc_get_heap_size() -> int64_t;
auto gc_get_used_size() -> int64_t;
void gc_collect();


} // namespace mono
