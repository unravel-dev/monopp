#pragma once

#include "mono_config.h"

#include "mono_assembly.h"

namespace mono
{

struct compiler_paths
{
	std::string assembly_dir;
	std::string config_dir;
	std::string msc_executable;
};

struct debugging_config
{
	bool enable_debugging = false;
	std::string address = "127.0.0.1";
	uint32_t port = 55555;
};

auto init(const compiler_paths& paths = {}, const debugging_config& debugging = {}) -> bool;

void shutdown();

struct compiler_params
{
	// Specifies output assembly name
	std::string output_name;

	std::string output_doc_name;

	// Specifies the input files to compile
	std::vector<std::string> files;

	// Specifies the format of the output assembly
	// Can be one of: exe, winexe, library, module
	std::string output_type = "library";

	// Everything below is optional

	// Imports metadata from the specified assemblies
	std::vector<std::string> references;

	// Specifies the location of referenced assemblies
	std::vector<std::string> references_locations;

	// Defines one or more conditional symbols
	std::vector<std::string> defines;

	// Debug or Optimized?
	bool debug{true};

	// Unsafe mode
	bool unsafe{true};
};

struct compile_cmd
{
	std::string cmd;
	std::vector<std::string> args;
};

auto create_compile_command(const compiler_params& params) -> std::string;
auto create_compile_command_detailed(const compiler_params& params) -> compile_cmd;
auto compile(const compiler_params& params) -> bool;

auto get_common_library_names() -> const std::vector<std::string>&;
auto get_common_library_paths() -> const std::vector<std::string>&;
// same count as library paths
auto get_common_config_paths() -> const std::vector<std::string>&;

auto get_common_executable_names() -> const std::vector<std::string>&;
auto get_common_executable_paths() -> const std::vector<std::string>&;


} // namespace mono
