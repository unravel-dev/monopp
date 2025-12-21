#include "monopp_suite.h"

#include <chrono>
#include <iostream>
#include <monopp/mono_assembly.h>
#include <monopp/mono_domain.h>
#include <monopp/mono_field_invoker.h>
#include <monopp/mono_internal_call.h>
#include <monopp/mono_jit.h>
#include <monopp/mono_method_invoker.h>
#include <monopp/mono_object.h>
#include <monopp/mono_property_invoker.h>
#include <monopp/mono_string.h>
#include <monopp/mono_type.h>
#include <suitepp/suite.hpp>

namespace monopp
{

void MyObject_CreateInternal(const mono::mono_object& this_ptr, float x, const std::string& v)
{
	mono::ignore(this_ptr, x, v);
	// std::cout << "FROM C++ : MyObject created." << std::endl;
}

void MyObject_DestroyInternal(const mono::mono_object& this_ptr)
{
	mono::ignore(this_ptr);
	// std::cout << "FROM C++ : MyObject deleted." << std::endl;
}

void MyObject_DoStuff(const mono::mono_object& this_ptr, const std::string& value)
{
	mono::ignore(this_ptr, value);
	// std::cout << "FROM C++ : DoStuff was called with: " << value << std::endl;
}

std::string MyObject_ReturnAString(const mono::mono_object& this_ptr, const std::string& value)
{
	mono::ignore(this_ptr, value);
	// std::cout << "FROM C++ : ReturnAString was called with: " << value << std::endl;
	return "The value: " + value;
}

void test_suite()
{
	mono::mono_domain domain("domain");
	mono::mono_domain::set_current_domain(domain);

	TEST_CASE("load invalid assembly")
	{
		EXPECT_THROWS_AS(domain.get_assembly("doesnt_exist_12345.dll"), mono::mono_exception);
	};

	TEST_CASE("jit compile assembly")
	{
		mono::compiler_params cmd;
		cmd.files = {DATA_DIR"managed/tests.cs"};
		cmd.references = {DATA_DIR"monort_managed.dll"};
		cmd.output_name = DATA_DIR"tests_managed.dll";

		bool jit_compile_result = mono::compile(cmd);

		EXPECT(jit_compile_result == true);
	};

	TEST_CASE("load valid assembly")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto refs = assembly.dump_references();
			for(const auto& ref : refs)
			{
				std::cout << ref << std::endl;
			}
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("load valid assembly and bind")
	{
		auto expression = [&]()
		{
			mono::add_internal_call("Tests.MyObject::CreateInternal",
									internal_call(MyObject_CreateInternal));
			mono::add_internal_call("Tests.MyObject::DestroyInternal",
									internal_call(MyObject_DestroyInternal));
			mono::add_internal_call("Tests.MyObject::DoStuff", internal_call(MyObject_DoStuff));
			mono::add_internal_call("Tests.MyObject::ReturnAString", internal_call(MyObject_ReturnAString));
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get invalid type")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");

			auto type = assembly.get_type("SometypeThatDoesntExist12345");
			EXPECT(!type.valid());
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get monopp valid type")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			EXPECT(type.valid());
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get valid method")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");

			auto method1 = mono::make_method_invoker<void()>(type, "Method1");

			auto method2 = mono::make_method_invoker<void(std::string)>(type, "Method2");

			auto method3 = mono::make_method_invoker<void(int)>(type, "Method3");

			auto method4 = mono::make_method_invoker<void(int, int)>(type, "Method4");

			auto method5 = mono::make_method_invoker<std::string(std::string, int)>(type, "Method5");
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get/set field")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");

			auto type = assembly.get_type("Tests", "MonoppTest");

			auto field = type.get_field("someField");

			auto mutable_field = mono::make_field_invoker<int>(field);

			auto obj = type.new_instance();

			auto some_field = mutable_field.get_value(obj);

			EXPECT(some_field == 12);

			int arg = 6;
			mutable_field.set_value(obj, arg);

			some_field = mutable_field.get_value(obj);
			EXPECT(some_field == 6);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get/set static field")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");

			auto field = type.get_field("someFieldStatic");

			auto mutable_field = mono::make_field_invoker<int>(field);

			auto some_field = mutable_field.get_value();

			EXPECT(some_field == 12);

			int arg = 6;
			mutable_field.set_value(arg);

			some_field = mutable_field.get_value();
			EXPECT(some_field == 6);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get invalid field")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			EXPECT_THROWS(type.get_field("someInvalidField"));
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get/set property")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");

			auto prop = type.get_property("someProperty");

			auto mutable_prop = mono::make_property_invoker<int>(prop);

			auto obj = type.new_instance();

			EXPECT(obj.valid());

			auto some_prop = mutable_prop.get_value(obj);

			EXPECT(some_prop == 12);

			int arg = 55;
			mutable_prop.set_value(obj, arg);

			some_prop = mutable_prop.get_value(obj);
			EXPECT(some_prop == 55);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get/set static property")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto prop = type.get_property("somePropertyStatic");
			auto mutable_prop = mono::make_property_invoker<int>(prop);
			auto some_prop = mutable_prop.get_value();
			EXPECT(some_prop == 6);

			int arg = 55;
			mutable_prop.set_value(arg);

			some_prop = mutable_prop.get_value();
			EXPECT(some_prop == 55);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("get invalid property")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			EXPECT_THROWS(type.get_property("someInvalidProperty"));
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 1")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<int(int)>(type, "Function1");
			const auto number = 1000;
			auto result = method_thunk(number);
			EXPECT(number + 1337 == result);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 2")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<void(float, int, float)>(type, "Function2");
			method_thunk(13.37f, 42, 9000.0f);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 3")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<void(std::string)>(type, "Function3");
			method_thunk("Hello!");
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 4")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<std::string(std::string)>(type, "Function4");
			auto expected_string = std::string("Hello!");
			auto result = method_thunk(expected_string);
			EXPECT(result == std::string("The string value was: " + expected_string));
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 5")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<void()>(type, "Function5");
			EXPECT_THROWS(method_thunk());
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call static method 6")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto method_thunk = mono::make_method_invoker<void()>(type, "Function6");
			method_thunk();
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call member method 1")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto obj = type.new_instance();
			auto method_thunk = mono::make_method_invoker<void()>(type, "Method1");
			method_thunk(obj);
		};
		EXPECT_NOTHROWS(expression());
	};

	TEST_CASE("call member method 2")
	{
		auto expression = [&]()
		{
			auto assembly = domain.get_assembly(DATA_DIR "tests_managed.dll");
			auto type = assembly.get_type("Tests", "MonoppTest");
			auto obj = type.new_instance();
			auto method_thunk = mono::make_method_invoker<std::string(std::string, int)>(type, "Method5");
			auto result = method_thunk(obj, "test", 5);
			EXPECT(result == std::string("Return Value: test"));
		};
		EXPECT_NOTHROWS(expression());
	};
}
} // namespace monopp
