cmake_minimum_required(VERSION 3.28)

foreach(required_argument IN ITEMS
        INPUT_ROOT OUTPUT_GUARD_ROOT OUTPUT_ROOT OUTPUT_HEADER)
    if(NOT DEFINED ${required_argument} OR "${${required_argument}}" STREQUAL "")
        message(FATAL_ERROR "${required_argument} is required")
    endif()
endforeach()

cmake_path(ABSOLUTE_PATH INPUT_ROOT NORMALIZE OUTPUT_VARIABLE normalized_input_root)
cmake_path(ABSOLUTE_PATH OUTPUT_GUARD_ROOT NORMALIZE
    OUTPUT_VARIABLE normalized_output_guard_root)
cmake_path(ABSOLUTE_PATH OUTPUT_ROOT NORMALIZE OUTPUT_VARIABLE normalized_output_root)
cmake_path(ABSOLUTE_PATH OUTPUT_HEADER NORMALIZE OUTPUT_VARIABLE normalized_output_header)
cmake_path(GET normalized_output_guard_root ROOT_PATH output_guard_root_path)
cmake_path(GET normalized_output_root ROOT_PATH output_root_path)
cmake_path(GET normalized_output_guard_root FILENAME output_guard_name)
cmake_path(GET normalized_output_guard_root PARENT_PATH output_guard_parent)
cmake_path(GET output_guard_parent FILENAME output_guard_parent_name)
cmake_path(IS_PREFIX normalized_output_guard_root "${normalized_output_root}"
    NORMALIZE output_is_guarded)
cmake_path(IS_PREFIX normalized_output_guard_root "${normalized_output_header}"
    NORMALIZE header_is_guarded)
if(normalized_output_guard_root STREQUAL output_guard_root_path OR
   normalized_output_guard_root STREQUAL normalized_input_root OR
   NOT output_guard_parent_name STREQUAL "generated" OR
   NOT output_guard_name STREQUAL "launcher" OR
   NOT output_is_guarded OR
   normalized_output_root STREQUAL normalized_output_guard_root OR
   normalized_output_root STREQUAL output_root_path OR
   NOT header_is_guarded OR
   normalized_output_header STREQUAL normalized_output_guard_root)
    message(FATAL_ERROR "Refusing to stage launcher resources into an unsafe output root")
endif()
if(NOT IS_DIRECTORY "${normalized_input_root}")
    message(FATAL_ERROR "Durango launcher input root does not exist: ${normalized_input_root}")
endif()

string(ASCII 9 manifest_tab)
string(ASCII 10 manifest_newline)
string(ASCII 13 carriage_return)

function(collect_launcher_files output_variable relative_root)
    file(GLOB_RECURSE collected_files LIST_DIRECTORIES false
        RELATIVE "${normalized_input_root}"
        "${normalized_input_root}/${relative_root}/*")
    list(FILTER collected_files EXCLUDE REGEX "(^|/)\\.DS_Store$")
    if(NOT collected_files)
        message(FATAL_ERROR
            "Durango launcher payload category is empty: ${relative_root}")
    endif()
    set(${output_variable} "${collected_files}" PARENT_SCOPE)
endfunction()

collect_launcher_files(launcher_scripts "scripts/ui")
collect_launcher_files(launcher_textures "textures")
collect_launcher_files(launcher_terrain "terrain")
collect_launcher_files(launcher_sounds "sounds/ui")

set(launcher_sources
    "ScaledWorldv4.7.rbxl"
    ${launcher_scripts}
    ${launcher_textures}
    ${launcher_terrain}
    ${launcher_sounds})
list(REMOVE_DUPLICATES launcher_sources)

set(launcher_mappings)
foreach(source_relative IN LISTS launcher_sources)
    string(FIND "${source_relative}" "|" pipe_index)
    string(FIND "${source_relative}" "${manifest_tab}" tab_index)
    string(FIND "${source_relative}" "${manifest_newline}" newline_index)
    string(FIND "${source_relative}" "${carriage_return}" carriage_return_index)
    if(NOT pipe_index EQUAL -1 OR NOT tab_index EQUAL -1 OR
       NOT newline_index EQUAL -1 OR NOT carriage_return_index EQUAL -1 OR
       source_relative MATCHES "(^|/)\\.\\.?(/|$)")
        message(FATAL_ERROR
            "Durango launcher source path cannot be represented safely: ${source_relative}")
    endif()

    set(source_path "${normalized_input_root}/${source_relative}")
    if(IS_SYMLINK "${source_path}" OR NOT EXISTS "${source_path}" OR
       IS_DIRECTORY "${source_path}")
        message(FATAL_ERROR
            "Durango launcher source must be a regular non-symlink file: ${source_relative}")
    endif()

    if(source_relative MATCHES "^scripts/ui/")
        string(REGEX REPLACE "^scripts/ui/" "scripts/"
            logical_path "${source_relative}")
    else()
        set(logical_path "${source_relative}")
    endif()
    list(APPEND launcher_mappings "${logical_path}|${source_relative}")
endforeach()
list(SORT launcher_mappings)

file(REMOVE_RECURSE "${normalized_output_root}")
file(MAKE_DIRECTORY "${normalized_output_root}/content")

list(LENGTH launcher_mappings launcher_entry_count)
string(CONCAT manifest_contents
    "ROBLOX_DURANGO_LAUNCHER_MANIFEST_V1${manifest_newline}"
    "entries${manifest_tab}${launcher_entry_count}${manifest_newline}")
set(previous_logical_path "")
foreach(mapping IN LISTS launcher_mappings)
    string(FIND "${mapping}" "|" separator_index)
    if(separator_index LESS 1)
        message(FATAL_ERROR "Invalid internal launcher mapping: ${mapping}")
    endif()
    string(SUBSTRING "${mapping}" 0 ${separator_index} logical_path)
    math(EXPR source_index "${separator_index} + 1")
    string(SUBSTRING "${mapping}" ${source_index} -1 source_relative)
    if(logical_path STREQUAL previous_logical_path)
        message(FATAL_ERROR "Duplicate staged launcher path: ${logical_path}")
    endif()
    set(previous_logical_path "${logical_path}")

    set(source_path "${normalized_input_root}/${source_relative}")
    set(staged_path "${normalized_output_root}/content/${logical_path}")
    cmake_path(GET staged_path PARENT_PATH staged_parent)
    file(MAKE_DIRECTORY "${staged_parent}")
    file(COPY_FILE "${source_path}" "${staged_path}" ONLY_IF_DIFFERENT)

    file(SIZE "${source_path}" source_size)
    file(SHA256 "${source_path}" source_sha256)
    file(SIZE "${staged_path}" staged_size)
    file(SHA256 "${staged_path}" staged_sha256)
    if(NOT source_size EQUAL staged_size OR
       NOT source_sha256 STREQUAL staged_sha256)
        message(FATAL_ERROR
            "Staged Durango launcher file failed size/SHA-256 verification: ${logical_path}")
    endif()
    string(APPEND manifest_contents
        "${source_sha256}${manifest_tab}${source_size}${manifest_tab}"
        "${logical_path}${manifest_newline}")
endforeach()

set(manifest_path "${normalized_output_root}/launcher-manifest.v1")
file(WRITE "${manifest_path}" "${manifest_contents}")
file(SHA256 "${manifest_path}" manifest_sha256)

cmake_path(GET normalized_output_header PARENT_PATH output_header_parent)
file(MAKE_DIRECTORY "${output_header_parent}")
file(WRITE "${normalized_output_header}"
    "#pragma once\n"
    "\n"
    "#include <cstddef>\n"
    "\n"
    "namespace rbx::player::launcher_manifest\n"
    "{\n"
    "inline constexpr char sha256[] = \"${manifest_sha256}\";\n"
    "inline constexpr std::size_t entryCount = ${launcher_entry_count};\n"
    "}\n")
