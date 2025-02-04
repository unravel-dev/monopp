#pragma once

#include "mono_config.h"
#include "mono_domain.h"
#include "mono_exception.h"
#include "mono_object.h"
#include "mono_type_traits.h"

#include <cstring>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

namespace mono
{

//------------------------------------------------------------------------------
// A lightweight base for wrapping System.Collections.Generic.List<T> objects
//------------------------------------------------------------------------------
class mono_list_base : public mono_object
{
public:
	explicit mono_list_base(const mono_object& obj)
		: mono_object(obj)
	{
	}

	// Get the 'Count' property by invoking 'get_Count'
	auto size() const -> std::size_t
	{
		MonoObject* exc = nullptr;
		MonoObject* result = invoke_method("get_Count", nullptr, 0, &exc);
		if(exc)
			throw mono_exception("Exception calling List<T>.get_Count");

		return static_cast<std::size_t>(*reinterpret_cast<int*>(mono_object_unbox(result)));
	}

	// Clear the list by invoking 'Clear'
	void clear()
	{
		MonoObject* exc = nullptr;
		invoke_method("Clear", nullptr, 0, &exc);
		if(exc)
			throw mono_exception("Exception calling List<T>.Clear()");
	}

protected:
	// Helper: invoke a method on 'this' object by name (naive approach)
	auto invoke_method(const char* methodName, void** params, int /*paramCount*/, MonoObject** exc) const
		-> MonoObject*
	{
		if(!object_)
			return nullptr;

		MonoClass* klass = mono_object_get_class(object_);
		void* iter = nullptr;
		MonoMethod* targetMethod = nullptr;

		while(MonoMethod* m = mono_class_get_methods(klass, &iter))
		{
			const char* name = mono_method_get_name(m);
			if(std::strcmp(name, methodName) == 0)
			{
				// (If you have multiple overloads, you'd also check paramCount, param types, etc.)
				targetMethod = m;
				break;
			}
		}

		if(!targetMethod)
			throw mono_exception(std::string("Method not found: ") + methodName);

		return mono_runtime_invoke(targetMethod, object_, params, exc);
	}
};

//------------------------------------------------------------------------------
// A lightweight wrapper for List<T> of certain trivial/primitive types
//------------------------------------------------------------------------------
template <typename T>
class mono_list : public mono_list_base
{
public:
	using value_type = T;

	static_assert(is_mono_valuetype<value_type>::value, "Not a value type");

	// 1) Construct from an existing MonoObject* that represents a List<T>
	//    e.g. you retrieved it from a field or method return value.
	explicit mono_list(const mono_object& obj)
		: mono_list_base(obj)
	{
	}

	auto craete_list(const mono_domain& domain) -> mono_object
	{
		mono_type list_class(get_list_class_for_type());
		if(list_class.valid())
		{
			return list_class.new_instance(domain);
		}
		else
		{
			return {};
		}
	}
	// 2) Construct a brand new List<T> in the given domain from a std::list<T>
	//    This uses a naive approach to locate the "System.Collections.Generic.List`1[System.Int32]"
	//    (or whichever T) class, then calls the default constructor, and then 'Add' for each item.
	mono_list(const mono_domain& domain, const std::list<T>& container)
		: mono_list_base(craete_list(domain))
	{
		if(!valid())
		{
			throw mono_exception("Exception creating List<T>");
		}
		// Create a new List<T> object in C#.
		// We'll do a naive approach for certain known T's (int, float, etc.).
		// If you need more advanced or custom T's, you'll need full generic reflection.

		// Now populate via Add(value)
		for(auto& item : container)
		{
			add(item);
		}
	}

	// Add an item to the end of the list (List<T>.Add)
	void add(const T& value)
	{
		// auto invoker = make_method_invoker<T>(get_type(), "Add");

		void* args[1];
		// For a primitive value, we can pass a pointer to it
		// (Mono runtime will unbox/box as needed).
		args[0] = (void*)&value;

		MonoObject* exc = nullptr;
		invoke_method("Add", args, 1, &exc);
		if(exc)
			throw mono_thunk_exception(exc);
	}

	// Retrieve an element by index (List<T>.get_Item)
	auto get(std::size_t index) const -> T
	{
		int idx = static_cast<int>(index); // method signature is int
		void* args[1] = {&idx};

		MonoObject* exc = nullptr;
		MonoObject* result = invoke_method("get_Item", args, 1, &exc);
		if(exc)
			throw mono_thunk_exception(exc);

		// For T=primitive, we unbox
		return *reinterpret_cast<T*>(mono_object_unbox(result));
	}

	// Set an element by index (List<T>.set_Item)
	void set(std::size_t index, const T& value)
	{
		int idx = static_cast<int>(index);
		void* args[2] = {&idx, (void*)&value};

		MonoObject* exc = nullptr;
		invoke_method("set_Item", args, 2, &exc);
		if(exc)
			throw mono_thunk_exception(exc);
	}

	// Convert managed List<T> to a std::list<T>
	auto to_list() const -> std::list<T>
	{
		std::list<T> result;
		std::size_t n = size();
		for(std::size_t i = 0; i < n; i++)
		{
			result.push_back(get(i));
		}
		return result;
	}

private:
	// Helper to get MonoClass for T without if constexpr
	static auto mono_class_from_element_type() -> MonoClass*
	{
		MonoClass* mono_type = nullptr;
		if(std::is_same<T, int8_t>::value)
		{
			mono_type = mono_get_sbyte_class();
		}
		if(std::is_same<T, int16_t>::value)
		{
			mono_type = mono_get_int16_class();
		}
		if(std::is_same<T, int32_t>::value)
		{
			mono_type = mono_get_int32_class();
		}
		if(std::is_same<T, int64_t>::value)
		{
			mono_type = mono_get_int64_class();
		}
		if(std::is_same<T, uint8_t>::value)
		{
			mono_type = mono_get_byte_class();
		}
		if(std::is_same<T, uint16_t>::value)
		{
			mono_type = mono_get_uint16_class();
		}
		if(std::is_same<T, uint32_t>::value)
		{
			mono_type = mono_get_uint32_class();
		}
		if(std::is_same<T, uint64_t>::value)
		{
			mono_type = mono_get_uint64_class();
		}
		else if(std::is_same<T, float>::value)
		{
			mono_type = mono_get_single_class();
		}
		else if(std::is_same<T, double>::value)
		{
			mono_type = mono_get_double_class();
		}
		else if(std::is_same<T, bool>::value)
		{
			mono_type = mono_get_boolean_class();
		}
		else if(std::is_same<T, char>::value)
		{
			mono_type = mono_get_char_class();
		}
		else if(std::is_same<T, std::string>::value)
		{
			mono_type = mono_get_string_class();
		}
		else
		{
			// mono_type = mono_get_byte_class();
		}
		return mono_type;
	}
	// Locate the closed generic type List<T> for certain trivial T's
	// using known Mono APIs (naive approach).
	static auto get_list_class_for_type() -> MonoClass*
	{
		// For demonstration, let's assume we only handle int32.
		// If T= int32_t => System.Int32
		// If T= float   => System.Single
		// ... you can extend with your own logic or a big if/else
		// like in your mono_array<T> code.

		// 1) Find the open generic List`1
		//    (In practice, you'd find it in mscorlib or netstandard, etc.)
		MonoImage* corlib = mono_get_corlib();
		// or use: MonoImage* corlib = mono_assembly_get_image(mono_get_corlib_assembly());

		MonoClass* openListClass = mono_class_from_name(corlib, "System.Collections.Generic", "List`1");
		if(!openListClass)
			return nullptr;

		// 2) Decide on the T. We'll do a small example:
		MonoClass* elementMonoClass = mono_class_from_element_type();
		if(!elementMonoClass)
		{
			return nullptr;
		}

		MonoType* elementMonoType = mono_class_get_type(elementMonoClass);

		// 3) Construct the closed generic List<T>
		// We do so by passing the elementMonoType as the single generic argument.
		// There's an official (somewhat tricky) way to do this with reflection APIs or
		// mono_class_bind_generic_parameters, etc.
		//
		// A simpler approach is to use reflection:
		//   - Create a System.Type object for T
		//   - Create a generic type from openListClass with that T
		//   - Then get the MonoClass for that closed type.
		//
		// For demonstration, here's one approach:

		// MonoReflectionType* rType = mono_type_get_object(mono_domain_get(), elementMonoType);
		// if(!rType)
		// 	return nullptr;

		// "List`1" -> MakeGenericType( T )
		// (C#-style reflection: typeof(List<>).MakeGenericType(new[] { typeof(T) })
		// MonoClass* gtdClass = mono_class_get_generic_class(openListClass);
		// if(!gtdClass)
		// 	return nullptr;

		// 	   // Reflection helper: mono_class_make_reflection_type() or mono_reflection_get_type()
		// 	   // etc. But there's also a simpler function in newer Mono called
		// 	   // The actual API usage can vary by Mono version. We'll do a typical reflection approach:

		// 	   // 4) We'll do a quick hack: use "System.Reflection.Emit" approach or
		// 	   //    official "mono_class_inflate_generic_class" if your Mono version supports it.

		// MonoGenericClass* genericClass = mono_class_get_generic_class(openListClass);
		// if(!genericClass)
		// 	return nullptr;

		// 4) Create a MonoGenericInst that holds our single type argument (int)
		MonoType* typeArgs[1];
		typeArgs[0] = elementMonoType;

		struct _MonoGenericInst
		{
			uint32_t id;			 /* unique ID for debugging */
			uint32_t type_argc : 22; /* number of type arguments */
			uint32_t is_open : 1;	 /* if this is an open type */
			MonoType* type_argv[1];
		};

		// This function is declared in "metadata.h":
		//   MonoGenericInst * mono_metadata_get_generic_inst (guint32 type_argc, MonoType **type_argv);
		// MonoGenericInst* genericInst = mono_metadata_get_generic_inst(1, typeArgs);
		// if (!genericInst) {
		// 	// handle error
		// }

		_MonoGenericInst clsctx;
		clsctx.type_argc = 1;
		clsctx.is_open = 0;
		clsctx.type_argv[0] = elementMonoType;

		struct _MonoGenericContext
		{
			/* The instantiation corresponding to the class generic parameters */
			_MonoGenericInst* class_inst;
			/* The instantiation corresponding to the method generic parameters */
			void* method_inst;
		};

		// 5) Build a MonoGenericContext referencing that inst for class instantiation
		_MonoGenericContext context;
		memset(&context, 0, sizeof(context));
		// We'll inflate the class type (not method instantiation), so set 'class_inst':
		context.class_inst = &clsctx;
		context.method_inst = nullptr; // no method generics here

		MonoType* closedListType = mono_class_inflate_generic_type(mono_class_get_type(openListClass),
																   (MonoGenericContext*)&context);
		if(!closedListType)
			return nullptr;

		// Finally get the MonoClass from that inflated type
		MonoClass* closedListClass = mono_class_from_mono_type(closedListType);
		return closedListClass;
	}
};

//------------------------------------------------------------------------------
// Specialization for mono_list<mono_object> (reference types):
// You could do something similar to handle user-defined classes, etc.
//------------------------------------------------------------------------------
template <>
class mono_list<mono_object> : public mono_list_base
{
public:
	explicit mono_list(const mono_object& obj)
		: mono_list_base(obj)
	{
	}

	// For demonstration, we won't implement the new-from-list constructor
	// because that requires building a generic List<object> type (and maybe an array of objects).
	// But if you needed it, you'd do it similarly: create a List<object>, call Add, etc.

	// size() and clear() come from base class

	void add(const mono_object& obj)
	{
		void* args[1] = {(void*)obj.get_internal_ptr()};
		MonoObject* exc = nullptr;
		invoke_method("Add", args, 1, &exc);
		if(exc)
			throw mono_thunk_exception(exc);
	}

	auto get(std::size_t index) const -> mono_object
	{
		int idx = static_cast<int>(index);
		void* args[1] = {&idx};
		MonoObject* exc = nullptr;
		MonoObject* result = invoke_method("get_Item", args, 1, &exc);
		if(exc)
			throw mono_thunk_exception(exc);

		return mono_object(result);
	}

	void set(std::size_t index, const mono_object& value)
	{
		int idx = static_cast<int>(index);
		void* args[2] = {&idx, (void*)value.get_internal_ptr()};
		MonoObject* exc = nullptr;
		invoke_method("set_Item", args, 2, &exc);
		if(exc)
			throw mono_thunk_exception(exc);
	}

	auto to_list() const -> std::list<mono_object>
	{
		std::list<mono_object> result;
		std::size_t n = size();
		for(std::size_t i = 0; i < n; i++)
		{
			result.push_back(get(i));
		}
		return result;
	}
};

} // namespace mono
