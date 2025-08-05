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

# Project options
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_COMPILE_FLAGS "Enable compile flags" ON)
option(ENABLE_STRIP "Enable symbol stripping for Release builds" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)
option(ENABLE_CCACHE "Enable ccache for faster builds" ON)

# Enforce out-of-source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use a separate build directory.")
endif()

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Enable Link Time Optimization (if supported and requested)
if(ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
  if(lto_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "Link Time Optimization (LTO) enabled for Release builds.")
  else()
    message(WARNING "Link Time Optimization (LTO) not supported: ${lto_error}.")
  endif()
endif()

# Optionally enable ccache for faster builds
if(ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "ccache found and enabled for faster builds.")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  else()
    message(STATUS "ccache not found, consider installing it to speed up rebuilds.")
  endif()
endif()

# Include project-specific CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(Flags)
include(External)

# Get the project version using Git tags if available, else default to "unknown"
set(PROJECT_VERSION "unknown")
if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
  find_package(Git QUIET)
  if(Git_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --always --dirty
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_DESCRIBE_OUTPUT)
      set(PROJECT_VERSION "${GIT_DESCRIBE_OUTPUT}")
      message(STATUS "Project version set to '${PROJECT_VERSION}' from Git.")
    else()
      message(STATUS "Failed to retrieve Git tag, using fallback version '${PROJECT_VERSION}'.")
    endif()
  else()
    message(STATUS "Failed to retrieve Git tag, using fallback version '${PROJECT_VERSION}'.")
  endif()
else()
  message(STATUS "Not a Git repository, using fallback version '${PROJECT_VERSION}'.")
endif()

# Generate the version header using the inferred Git tag version
configure_file("${PROJECT_SOURCE_DIR}/src/generated.hpp.in" "${PROJECT_BINARY_DIR}/generated/generated.hpp" @ONLY)

# Create main library target
add_library(${PROJECT_NAME}-lib STATIC
  # find src -name "*.cpp" ! -name "main.cpp" | sort
  src/app.cpp
  src/assets/sounds.cpp
  src/assets/textures.cpp
  src/core/backend.cpp
  src/core/gamepad.cpp
  src/core/imgui_sfml_ctx.cpp
  src/core/io.cpp
  src/core/platform/posix.cpp
  src/core/platform/windows.cpp
  src/core/widgets.cpp
  src/core/world.cpp
  src/game/entities.cpp
)

# Include headers relatively to the src directory, include generated headers
target_include_directories(${PROJECT_NAME}-lib PUBLIC src "${PROJECT_BINARY_DIR}/generated")

# Apply public compile flags to the library target if enabled
if(ENABLE_COMPILE_FLAGS)
  apply_compile_flags(${PROJECT_NAME}-lib)
endif()

# Fetch and link external dependencies to the library target
fetch_and_link_external_dependencies(${PROJECT_NAME}-lib)

# Add the main executable and link the library
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-lib)

# Prevent the creation of a console window on Windows and create an application bundle on macOS
# If "WIN32_EXECUTABLE" is set to ON, the "SFML::Main" library must be linked to the target, which is done in "cmake/External.cmake"
set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)

# Platform-specific symbol stripping for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_STRIP)
  if(APPLE)
    # macOS - strip symbols using post-build command
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND strip -x "$<TARGET_FILE:${PROJECT_NAME}>"
      COMMENT "Stripping symbols for Release build on macOS."
    )
    message(STATUS "Symbol stripping enabled for Release builds on macOS.")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux - use linker flag
    target_link_options(${PROJECT_NAME} PRIVATE "-s")
    message(STATUS "Symbol stripping enabled for Release builds on Linux.")
  elseif(MSVC)
    # Windows (MSVC) - use linker optimization flags
    # I use MSVC in CI/CD, so this is good enough for me
    target_link_options(${PROJECT_NAME} PRIVATE
      "/DEBUG:NONE"
      "/OPT:REF"
      "/OPT:ICF"
    )
    message(STATUS "Symbol stripping enabled for Release builds on MSVC.")
  else()
    # Other platforms, probably Windows (MinGW) or FreeBSD
    message(STATUS "Symbol stripping not configured for this platform.")
  endif()
else()
  if(NOT ENABLE_STRIP)
    message(STATUS "Symbol stripping disabled by user.")
  else()
    message(STATUS "Symbol stripping not applied (non-Release build).")
  endif()
endif()

# Set extra platform-specific configurations
if(APPLE)
  # If on macOS, setup the app bundle properties directly on target

  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/assets/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.ryouze.${PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "ryouze.net"
    MACOSX_BUNDLE_ICON_FILE "Icon"
  )

  # Copy the icon into the app bundle
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${PROJECT_SOURCE_DIR}/cmake/assets/Icon.icns"
      "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/Icon.icns"
    COMMENT "Copying icon into the macOS app bundle."
  )
elseif(WIN32)
  # If on Windows, add an icon to the executable

  # Generate RC file for Windows icon
  # The macro is not used for setting the titlebar icon, we start the game in fullscreen mode
  file(GENERATE
    OUTPUT "${PROJECT_BINARY_DIR}/generated/icon.rc"
    CONTENT "#define IDI_ICON1 101\n\nIDI_ICON1 ICON \"${PROJECT_SOURCE_DIR}/cmake/assets/Icon.ico\""
  )

  # Add the generated .rc file to the target
  target_sources(${PROJECT_NAME} PRIVATE "${PROJECT_BINARY_DIR}/generated/icon.rc")
endif()

# Add install target
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION "/Applications"             # macOS app bundle
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"  # Executable on Linux/Windows
)

# Add tests if enabled
if(BUILD_TESTS)
  message(STATUS "Tests are enabled, creating test executable...")
  # Enable testing with CTest
  enable_testing()

  # Add test executable
  add_executable(tests
    # find tests -name "*.cpp" | sort
    tests/core/platform.test.cpp
    tests/core/world.test.cpp
    tests/game/entities.test.cpp
  )
  target_link_libraries(tests PRIVATE ${PROJECT_NAME}-lib)

  # Download and link test dependencies
  fetch_and_link_external_test_dependencies("tests")

  # Register tests separately with CTest
  #add_test(NAME "[src][assets]" COMMAND tests "[src][assets]")
  add_test(NAME "[src][core]" COMMAND tests "[src][core]")
  add_test(NAME "[src][game]" COMMAND tests "[src][game]")
endif()

# Print comprehensive build summary
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "                              BUILD CONFIGURATION")
message(STATUS "================================================================================")
message(STATUS "")
message(STATUS "Project Information:")
message(STATUS "  Project Name ............... ${PROJECT_NAME}")
message(STATUS "  Version .................... ${PROJECT_VERSION}")
message(STATUS "  Build Type ................. ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard ............... C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Shared Libraries ........... ${BUILD_SHARED_LIBS}")
message(STATUS "")
message(STATUS "Compiler & Toolchain:")
message(STATUS "  Compiler ID ................ ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Compiler Version ........... ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Compile Commands ........... ${CMAKE_EXPORT_COMPILE_COMMANDS}")
message(STATUS "  Extensions ................. ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "")
message(STATUS "Build Optimization:")
message(STATUS "  Compile Flags .............. ${ENABLE_COMPILE_FLAGS}")
message(STATUS "  Link Time Opt (LTO) ........ ${ENABLE_LTO}")
message(STATUS "  Symbol Stripping ........... ${ENABLE_STRIP}")
message(STATUS "  ccache ..................... ${ENABLE_CCACHE}")
message(STATUS "")
message(STATUS "Platform Information:")
message(STATUS "  System Name ................ ${CMAKE_SYSTEM_NAME}")
message(STATUS "  System Version ............. ${CMAKE_SYSTEM_VERSION}")
message(STATUS "  System Processor ........... ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Generator .................. ${CMAKE_GENERATOR}")
message(STATUS "")
message(STATUS "Directories:")
message(STATUS "  Source Directory ........... ${PROJECT_SOURCE_DIR}")
message(STATUS "  Binary Directory ........... ${PROJECT_BINARY_DIR}")
message(STATUS "  Install Prefix ............. ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "")
