#include "mono_jit.h"
#include "mono_assembly.h"
#include "mono_exception.h"
#include "mono_logger.h"

BEGIN_MONO_INCLUDE
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>
#include <mono/utils/mono-logger.h>
#include <mono_build_config.h>
END_MONO_INCLUDE

#include <iostream>

namespace mono
{

static void on_log_callback(const char* log_domain, const char* log_level, const char* message,
							mono_bool /*fatal*/, void* /*user_data*/)

{

	//	static const char* mono_error_levels[] = {"error", "critical", "warning",
	//											"message", "info",  "debug"};

	std::string category;
	if(log_level == nullptr)
	{
		category = "warning";
	}
	std::string format_msg;
	if(log_domain)
	{
		format_msg += "[";
		format_msg += log_domain;
		format_msg += "] ";
	}
	format_msg += message;
	const auto& logger = get_log_handler(category);
	if(logger)
	{
		logger(format_msg);
	}
	else
	{
		const auto& default_logger = get_log_handler("default");
		if(default_logger)
		{
			default_logger(format_msg);
		}
	}
}

static MonoDomain* jit_domain = nullptr;
static compiler_paths comp_paths{};

auto mono_assembly_dir() -> const char*
{
	return comp_paths.assembly_dir.empty() ? INTERNAL_MONO_ASSEMBLY_DIR : comp_paths.assembly_dir.c_str();
}

auto mono_config_dir() -> const char*
{
	return comp_paths.config_dir.empty() ? INTERNAL_MONO_CONFIG_DIR : comp_paths.config_dir.c_str();
}

auto mono_msc_executable() -> const char*
{
	return comp_paths.msc_executable.empty() ? INTERNAL_MONO_MCS_EXECUTABLE : comp_paths.msc_executable.c_str();
}

auto get_common_library_names() -> const std::vector<std::string>&
{
#ifdef _WIN32
	static const std::vector<std::string> names{"mono-2.0", "monosgen-2.0", "mono-2.0-sgen"};
#else
	static const std::vector<std::string> names{"libmono-2.0", "libmonosgen-2.0", "libmono-2.0-sgen"};
#endif
	return names;
}
auto get_common_library_paths() -> const std::vector<std::string>&
{
	static const std::vector<std::string> paths{"C:/Program Files/Mono/lib",
												"/usr/lib64",
												"/usr/lib",
												"/usr/local/lib64",
												"/usr/local/lib",
												"/opt/local/lib"};
	return paths;
}

auto get_common_executable_names() -> const std::vector<std::string>&
{
#ifdef _WIN32
	static const std::vector<std::string> names{"mcs.bat"};
#else
	static const std::vector<std::string> names{"mcs"};

#endif

	return names;
}
auto get_common_executable_paths() -> const std::vector<std::string>&
{
	static const std::vector<std::string> paths{
		"C:/Program Files/Mono/bin",
		"/bin",
		"/usr/bin",
		"/usr/local/bin"
	};
	return paths;
}


auto get_common_config_paths() -> const std::vector<std::string>&
{
	static const std::vector<std::string> paths{"C:/Program Files/Mono/etc",
												"/usr/etc",
												"/usr/local/etc",
												"/opt/local/etc"};
	return paths;
}



auto init(const compiler_paths& paths, bool enable_debugging) -> bool
{
	comp_paths = paths;

	mono_set_dirs(mono_assembly_dir(), mono_config_dir());

	if(enable_debugging)
	{
		// clang-format off
		const char* options[] =
		{
			"--soft-breakpoints",
			"--debugger-agent=transport=dt_socket,suspend=n,server=y,address=127.0.0.1:55555,embedding=1",
			"--debug-domain-unload",

			// GC options:
			// check-remset-consistency: Makes sure that write barriers are properly issued in native code,
			// and therefore
			//    all old->new generation references are properly present in the remset. This is easy to mess
			//    up in native code by performing a simple memory copy without a barrier, so it's important to
			//    keep the option on.
			// verify-before-collections: Unusure what exactly it does, but it sounds like it could help track
			// down
			//    things like accessing released/moved objects, or attempting to release handles for an
			//    unloaded domain.
			// xdomain-checks: Makes sure that no references are left when a domain is unloaded.
			"--gc-debug=check-remset-consistency,verify-before-collections,xdomain-checks"
		};
		// clang-format on
		mono_jit_parse_options(sizeof(options) / sizeof(char*), const_cast<char**>(options));
		mono_debug_init(MONO_DEBUG_FORMAT_MONO);
	}

	mono_trace_set_level_string("warning");
	mono_trace_set_log_handler(on_log_callback, nullptr);

	set_log_handler("default", [](const std::string& msg) { std::cout << msg << std::endl; });

	jit_domain = mono_jit_init("mono_jit");
	mono_thread_set_main(mono_thread_current());

	return jit_domain != nullptr;
}

void shutdown()
{
	if(jit_domain)
	{
		mono_jit_cleanup(jit_domain);
	}
	jit_domain = nullptr;
}

auto quote(const std::string& word) -> std::string
{
	std::string command;
	command.append("\"");
	command.append(word);
	command.append("\"");
	return command;
}


auto create_compile_command(const compiler_params& params) -> std::string
{
	std::string command;
	command.append(quote(mono_msc_executable()));

	for(const auto& path : params.files)
	{
		command += " ";
		command += quote(path);
	}

    if(!params.output_type.empty())
    {
        command += " -target:";
        command += params.output_type;
    }

    if(!params.references.empty())
    {
        command += " -reference:";

        for(const auto& ref : params.references)
        {
            command += ref;
            command += ",";
        }

        command.pop_back();
    }

    if(!params.references_locations.empty())
    {
        command += " -lib:";

        for(const auto& loc : params.references_locations)
        {
            command += loc;
            command += ",";
        }

        command.pop_back();
    }

	if(params.debug)
	{
		command += " -debug";
	}
	else
	{
		command += " -optimize";
	}

	if(params.unsafe)
	{
		command += " -unsafe";
	}

	command += " -out:";
	command += quote(params.output_name);

#ifdef _WIN32
	command = quote(command);
#endif
    return command;
}

auto create_compile_command_detailed(const compiler_params& params) -> compile_cmd
{
	compile_cmd cmd;
	cmd.cmd = mono_msc_executable();

	for(const auto& path : params.files)
	{
		//cmd.args.emplace_back(quote(path));
		cmd.args.emplace_back(path);

	}

	if(!params.output_type.empty())
	{
		std::string arg;
		arg += "-target:";
		arg += params.output_type;

		cmd.args.emplace_back(arg);
	}

	if(!params.references.empty())
	{
		std::string arg;
		arg += "-reference:";

		for(const auto& ref : params.references)
		{
			//arg += quote(ref);
			arg += ref;

			arg += ",";
		}

		arg.pop_back();
		cmd.args.emplace_back(arg);
	}

	if(!params.references_locations.empty())
	{
		std::string arg;
		arg += "-lib:";

		for(const auto& loc : params.references_locations)
		{
			//arg += quote(loc);
			arg += loc;

			arg += ",";
		}

		arg.pop_back();
		cmd.args.emplace_back(arg);
	}

	if(params.debug)
	{
		cmd.args.emplace_back("-debug");
	}
	else
	{
		cmd.args.emplace_back("-optimize");
	}

	if(params.unsafe)
	{
		cmd.args.emplace_back("-unsafe");
	}

	{
		std::string arg;

		arg += "-out:";
		//arg += quote(params.output_name);
		arg += params.output_name;

		cmd.args.emplace_back(arg);

	}

	return cmd;
}

auto compile(const compiler_params& params) -> bool
{
	auto command = create_compile_command(params);
    std::cout << command << std::endl;
	return std::system(command.c_str()) == 0;
}

} // namespace mono
