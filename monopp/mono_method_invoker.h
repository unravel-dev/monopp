#pragma once

#include "mono_config.h"

#include "mono_exception.h"
#include "mono_method.h"
#include "mono_object.h"
#include "mono_type.h"
#include "mono_type_conversion.h"

#include <tuple>
#include <utility>
#include <array>

namespace mono
{

template <typename T>
auto is_compatible_type(const mono_type& type) -> bool
{
	const auto& expected_name = type.get_fullname();
	return types::is_compatible_type<T>(expected_name);
}

template <typename Signature>
auto has_compatible_signature(const mono_method& method) -> bool
{
	constexpr auto arity = function_traits<Signature>::arity;
	using return_type = typename function_traits<Signature>::return_type;
	using arg_types = typename function_traits<Signature>::arg_types_decayed;
	auto expected_rtype = method.get_return_type();
	const auto& expected_arg_types = method.get_param_types();

	bool compatible = arity == expected_arg_types.size();
	if(!compatible)
	{
		return false;
	}
	compatible &= is_compatible_type<return_type>(expected_rtype);
	if(!compatible)
	{
		// allow cpp return type to be void i.e ignoring it.
		if(!is_compatible_type<void>(expected_rtype))
		{
			return false;
		}
	}
	arg_types tuple;
	size_t idx = 0;
	mono::for_each(tuple,
			 [&compatible, &idx, &expected_arg_types](const auto& arg)
			 {
				 mono::ignore(arg);
				 auto expected_arg_type = expected_arg_types[idx];
				 using arg_type = decltype(arg);
				 compatible &= is_compatible_type<arg_type>(expected_arg_type);

				 idx++;
			 });

	return compatible;
}

template <typename T>
class mono_method_invoker;

template <typename... Args>
class mono_method_invoker<void(Args...)> : public mono_method
{
public:
	void operator()(Args... args)
	{
		invoke(nullptr, std::forward<Args>(args)...);
	}

	void operator()(const mono_object& obj, Args... args)
	{
		invoke(&obj, std::forward<Args>(args)...);
	}

private:
	void invoke(const mono_object* obj, Args... args)
	{
		auto method = this->method_;
		MonoObject* object = nullptr;
		if(obj)
		{
			object = obj->get_internal_ptr();

			method = mono_object_get_virtual_method(object, method);
		}
		auto tup = std::make_tuple(mono_converter<std::decay_t<Args>>::to_mono(std::forward<Args>(args))...);

		const auto& param_types = this->get_param_types();
		auto inv = [&](auto... args)
		{
			constexpr size_t N = sizeof...(args);
			
			// Create args array with correct parameter types (C++14 compatible)
			std::array<void*, N> argsv;
			size_t idx = 0;
			auto fill_args = [&](auto&& arg) -> int {
				mono_type param_type = (idx < param_types.size()) ? param_types[idx] : mono_type{};
				argsv[idx] = to_mono_arg(arg, param_type);
				++idx;
				return 0;
			};
			// C++14 compatible parameter pack expansion using initializer list
			std::initializer_list<int> dummy = {fill_args(args)...};
			(void)dummy; // Suppress unused variable warning

			MonoObject* ex = nullptr;
			mono_runtime_invoke(method, object, argsv.data(), &ex);
			if(ex)
			{
				throw mono_thunk_exception(ex);
			}
		};

		mono::apply(inv, tup);
	}

	template <typename Signature>
	friend auto make_method_invoker(const mono_method&, bool) -> mono_method_invoker<Signature>;

	mono_method_invoker(const mono_method& o)
		: mono_method(o)
	{
	}
};

template <typename RetType, typename... Args>
class mono_method_invoker<RetType(Args...)> : public mono_method
{
public:
	auto operator()(Args... args)
	{
		return invoke(nullptr, std::forward<Args>(args)...);
	}

	auto operator()(const mono_object& obj, Args... args)
	{
		return invoke(&obj, std::forward<Args>(args)...);
	}

private:
	auto invoke(const mono_object* obj, Args... args)
	{
		auto method = this->method_;
		MonoObject* object = nullptr;
		if(obj)
		{
			object = obj->get_internal_ptr();

			method = mono_object_get_virtual_method(object, method);
		}
		auto tup = std::make_tuple(mono_converter<std::decay_t<Args>>::to_mono(std::forward<Args>(args))...);
		const auto& param_types = this->get_param_types();
		auto inv = [&](auto... args)
		{
			constexpr size_t N = sizeof...(args);
			
			// Create args array with correct parameter types (C++14 compatible)
			std::array<void*, N> argsv;
			size_t idx = 0;
			auto fill_args = [&](auto&& arg) -> int {
				mono_type param_type = (idx < param_types.size()) ? param_types[idx] : mono_type{};
				argsv[idx] = to_mono_arg(arg, param_type);
				++idx;
				return 0;
			};
			// C++14 compatible parameter pack expansion using initializer list
			std::initializer_list<int> dummy = {fill_args(args)...};
			(void)dummy; // Suppress unused variable warning

			MonoObject* ex = nullptr;
			auto result = mono_runtime_invoke(method, object, argsv.data(), &ex);
			if(ex)
			{
				throw mono_thunk_exception(ex);
			}

			return result;
		};

		auto result = mono::apply(inv, tup);
		return mono_converter<std::decay_t<RetType>>::from_mono(std::move(result));
	}

	template <typename Signature>
	friend auto make_method_invoker(const mono_method&, bool) -> mono_method_invoker<Signature>;

	mono_method_invoker(const mono_method& o)
		: mono_method(o)
	{
	}
};

template <typename Signature>
auto make_method_invoker(const mono_method& method, bool check_signature = true)
	-> mono_method_invoker<Signature>
{
	if(check_signature && !has_compatible_signature<Signature>(method))
	{
		throw mono_exception("NATIVE::Method thunk requested with incompatible signature");
	}
	return mono_method_invoker<Signature>(method);
}

template <typename Signature>
auto make_method_invoker(const mono_type& type, const std::string& name) -> mono_method_invoker<Signature>
{
	using arg_types = typename function_traits<Signature>::arg_types;
	auto args_result = types::get_args_signature<arg_types>();
	auto all_types_known = args_result.second;

	if(all_types_known)
	{
		auto func = type.get_method(name + "(" + args_result.first + ")");
		return make_method_invoker<Signature>(func);
	}
	else
	{
		constexpr auto arg_count = function_traits<Signature>::arity;
		auto func = type.get_method(name, arg_count);
		return make_method_invoker<Signature>(func);
	}
}

template <typename Signature>
auto make_method_invoker(const mono_object& obj, const std::string& name) -> mono_method_invoker<Signature>
{
	const auto& type = obj.get_type();

	return make_method_invoker<Signature>(type, name);
}

} // namespace mono
