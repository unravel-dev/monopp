
set(MONO_BINARY_PATCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/mono-6.12.0.206-patch)
include(Mono)

set(INTERNAL_MONO_MCS_EXECUTABLE "${MONO_MCS_EXECUTABLE}"
    CACHE PATH "Path to the Mono msc executable (mono/bin/msc). May be a relative path.")
set(INTERNAL_MONO_ASSEMBLY_DIR "${MONO_ASSEMBLY_PATH}"
    CACHE PATH "Path to the Mono assembly dir (mono/lib). May be a relative path.")
set(INTERNAL_MONO_CONFIG_DIR "${MONO_CONFIG_PATH}"
    CACHE PATH "Path to the Mono config dir (mono/etc). May be a relative path.")

configure_file(mono_build_config.h.in mono_build_config.h @ONLY)

set(target_name monopp)

file(GLOB_RECURSE libsrc *.h *.cpp *.hpp *.c *.cc)

add_library(${target_name} ${libsrc})

target_include_directories(${target_name}
    PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/..
        ${MONO_INCLUDE_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${target_name} PUBLIC ${MONO_LIBRARIES})

set_target_properties(${target_name} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_compile_definitions(${target_name} PUBLIC MONOPP_DEBUG_LEVEL=0)

include(target_warning_support)
set_warning_level(${target_name} ultra)

include(target_code_style_support)
set_code_style(${target_name} lower_case check_headers "${extra_flags}")
