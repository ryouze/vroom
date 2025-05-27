# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.28)

# Set project name and language
project(vroom LANGUAGES CXX)

# Set C++ standard to C++20, disable compiler-specific extensions and shared libraries
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Link Time Optimization (if supported)
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
if(lto_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled for Release builds.")
else()
  message(WARNING "Link Time Optimization (LTO) not supported: ${lto_error}")
endif()

# Project options
# option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COMPILE_FLAGS "Enable compile flags" ON)
option(ENABLE_STRIP "Enable symbol stripping for Release builds" ON)

# Enforce out-of-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use a separate build directory.")
endif()

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to 'Release' build type.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Include external CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include custom modules
include(Flags)
include(External)

# Optionally enable global ccache
find_program(CCACHE ccache)
if(CCACHE)
  message(STATUS "Ccache enabled for faster builds.")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else()
  message(WARNING "Ccache not found. Consider installing it to speed up rebuilds.")
endif()

# Get the project version using Git tags if available, else default to "unknown"
set(PROJECT_VERSION "unknown")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git REQUIRED)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(GIT_TAG)
    set(PROJECT_VERSION ${GIT_TAG})
    message(STATUS "Project version set to ${PROJECT_VERSION} from Git.")
  else()
    message(WARNING "Failed to retrieve Git tag. Using fallback version: ${PROJECT_VERSION}.")
  endif()
else()
  message(WARNING "Git repository not found. Using fallback version: ${PROJECT_VERSION}.")
endif()

# Generate the version header using the inferred Git tag version
configure_file(${CMAKE_SOURCE_DIR}/src/generated.hpp.in ${CMAKE_BINARY_DIR}/generated/generated.hpp @ONLY)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Add the main library target
add_library(${PROJECT_NAME}-lib STATIC
  # find src -name "*.cpp" ! -name "main.cpp" | sort
  src/app.cpp
  src/assets/textures.cpp
  src/core/backend.cpp
  src/core/game.cpp
  src/core/io.cpp
  src/core/misc.cpp
  src/core/platform/posix.cpp
  src/core/platform/windows.cpp
  src/core/ui.cpp
)

# Include headers relatively to the src directory
target_include_directories(${PROJECT_NAME}-lib PUBLIC src)

# Apply public compile flags to the library target if enabled
if(ENABLE_COMPILE_FLAGS)
  apply_compile_flags(${PROJECT_NAME}-lib)
endif()

# Fetch and link external dependencies to the library target
fetch_and_link_external_dependencies(${PROJECT_NAME}-lib)

# Add the main executable and link the library
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-lib)

# Strip symbols for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_STRIP)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND strip -x $<TARGET_FILE:${PROJECT_NAME}>
      COMMENT "Stripping symbols for Release builds on macOS."
    )
    message(STATUS "Stripping symbols for Release builds on macOS.")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # GNU/Linux
    target_link_options(${PROJECT_NAME} PRIVATE -s)
    message(STATUS "Stripping symbols for Release builds on GNU/Linux.")
  elseif(MSVC)
    # Windows (MSVC)
    # I use MSVC in CI/CD, so this is good enough for me
    target_link_options(${PROJECT_NAME} PRIVATE /DEBUG:NONE /OPT:REF /OPT:ICF)
    message(STATUS "Stripping symbols for Release builds on MSVC.")
  else()
    # Other platforms, probably Windows (MinGW) or FreeBSD
    message(WARNING "Stripping not configured for this platform.")
  endif()
else()
  if(NOT ENABLE_STRIP)
    message(STATUS "Symbol stripping is disabled by user.")
  else()
    message(STATUS "No stripping applied (non-Release builds).")
  endif()
endif()

# Prevent the creation of a console window on Windows and create an application bundle on macOS
# If "WIN32_EXECUTABLE" is set to ON, the "SFML::Main" library must be linked to the target, which is done in "cmake/External.cmake"
set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)

# If on macOS, bundle the executable into an app bundle
if(APPLE)
  # Set macOS bundle properties directly on target
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/assets/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.ryouze.${PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "ryouze.net"
    MACOSX_BUNDLE_ICON_FILE "Icon"
  )

  # Copy the icon into the app bundle
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/cmake/assets/Icon.icns"
      "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/Icon.icns"
    COMMENT "Copying icon to app bundle."
  )
elseif(WIN32)
  # If on Windows, add an icon to the executable
  # Specify the path to the .ico file
  set(ICON_PATH "${CMAKE_SOURCE_DIR}/cmake/assets/Icon.ico")

  # Generate the icon.rc file dynamically
  # Ensure that the macro's value in "src/core/windows.cpp" is also 101
  file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/generated/icon.rc" CONTENT "#define IDI_ICON1 101\n\nIDI_ICON1 ICON \"${ICON_PATH}\"")

  # Add the generated .rc file to the target
  target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/generated/icon.rc")
endif()

# Add install target
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION "/Applications"           # macOS
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # GNU/Linux, Windows
)

# # Add tests if enabled
# if(BUILD_TESTS)
#   message(STATUS "Tests enabled.")
# endif()

# Print the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}.")
