# include some defines automatically made by qpm
include(qpm_defines.cmake)

# bump android sdk version to 29 (10)
set(ANDROID_PLATFORM 29)

# override mod id
set(MOD_ID "graphicstweaks")

# Enable link time optimization
# In my experience, this can be highly unstable but it nets a huge size optimization and likely performance
# However, the instability was seen using Android.mk/ndk-build builds. With Ninja + CMake, this problem seems to have been solved.
# As always, test thoroughly
# - Fern
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# Hot reload (DO NOT USE IN PRODUCTION)
# add_compile_options(-DHotReload)

cmake_minimum_required(VERSION 3.21)
project(${COMPILE_ID})
include(${EXTERN_DIR}/includes/kaleb/shared/cmake/assets.cmake)

# export compile commands for significantly better intellisense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

# define that stores the actual source directory
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# compile options used
add_link_options(-Wl,--exclude-libs,ALL)
add_compile_options(-frtti -fexceptions -fvisibility=hidden)
# add_compile_options(-O0)

# get git info
execute_process(COMMAND git config user.name OUTPUT_VARIABLE GIT_USER)
execute_process(COMMAND git branch --show-current OUTPUT_VARIABLE GIT_BRANCH)
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT)
execute_process(COMMAND git diff-index --quiet HEAD RESULT_VARIABLE GIT_MODIFIED)

string(STRIP "${GIT_USER}" GIT_USER)
string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
string(STRIP "${GIT_COMMIT}" GIT_COMMIT)
string(STRIP "${GIT_MODIFIED}" GIT_MODIFIED)

message(STATUS "GIT_USER: ${GIT_USER}")
message(STATUS "GIT_BRANCH: ${GIT_BRANCH}")
message(STATUS "GIT_COMMIT: 0x${GIT_COMMIT}")
message(STATUS "GIT_MODIFIED: ${GIT_MODIFIED}")

# set git defines
add_compile_definitions(GIT_USER=\"${GIT_USER}\")
add_compile_definitions(GIT_BRANCH=\"${GIT_BRANCH}\")
add_compile_definitions(GIT_COMMIT=0x${GIT_COMMIT})
add_compile_definitions(GIT_MODIFIED=${GIT_MODIFIED})

# compile definitions used
add_compile_definitions(VERSION=\"${MOD_VERSION}\")
add_compile_definitions(MOD_ID=\"${MOD_ID}\")

# recursively get all src files
RECURSE_FILES(cpp_file_list ${SOURCE_DIR}/*.cpp)
RECURSE_FILES(c_file_list ${SOURCE_DIR}/*.c)

# add all src files to compile
add_library(
        ${COMPILE_ID}
        SHARED
        ${cpp_file_list}
        ${c_file_list}
)

target_include_directories(${COMPILE_ID} PRIVATE .)

# add src dir as include dir
target_include_directories(${COMPILE_ID} PRIVATE ${SOURCE_DIR})
# add include dir as include dir
target_include_directories(${COMPILE_ID} PRIVATE ${INCLUDE_DIR})
# add shared dir as include dir
target_include_directories(${COMPILE_ID} PUBLIC ${SHARED_DIR})
# codegen includes
target_include_directories(${COMPILE_ID} PRIVATE ${EXTERN_DIR}/includes/${CODEGEN_ID}/include)

# add assets
add_assets(${COMPILE_ID}-assets STATIC ${CMAKE_CURRENT_LIST_DIR}/assets ${INCLUDE_DIR}/assets.hpp)
target_link_libraries(${COMPILE_ID} PRIVATE -llog -lz  ${COMPILE_ID}-assets)

# add extern stuff like libs and other includes
include(extern.cmake)

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_STRIP} -d --strip-all
        "lib${COMPILE_ID}.so" -o "stripped_lib${COMPILE_ID}.so"
        COMMENT "Strip debug symbols done on final binary.")

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory debug
        COMMENT "Make directory for debug symbols"
        )

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename lib${COMPILE_ID}.so debug/lib${COMPILE_ID}.so
        COMMENT "Rename the lib to debug_ since it has debug symbols"
        )

# strip debug symbols from the .so and all dependencies
add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename stripped_lib${COMPILE_ID}.so lib${COMPILE_ID}.so
        COMMENT "Rename the stripped lib to regular"
        )
        foreach(so_file ${so_list})
        cmake_path(GET so_file FILENAME file)

        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${so_file} debug/${file}
                COMMENT "Copy so files for ndk stack"
        )

        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
                COMMAND ${CMAKE_STRIP} -g -S -d --strip-all ${so_file} -o ${file}
                COMMENT "Strip debug symbols from the dependencies")
        endforeach()

        foreach(a_file ${a_list})
        cmake_path(GET a_file FILENAME file)

        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${a_file} debug/${file}
        COMMENT "Copy a files for ndk stack")
        endforeach()

RECURSE_FILES(src_inline_hook_beatsaber_hook_local_extra_c ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.c)
RECURSE_FILES(src_inline_hook_beatsaber_hook_local_extra_cpp ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.cpp)

target_sources(${COMPILE_ID} PRIVATE ${src_inline_hook_beatsaber_hook_local_extra_c})
target_sources(${COMPILE_ID} PRIVATE ${src_inline_hook_beatsaber_hook_local_extra_cpp})
