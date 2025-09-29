include(FetchContent)

# Download and link base dependencies (graphics, logging, etc.)
function(fetch_and_link_external_dependencies target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist. Cannot fetch and link dependencies.")
  endif()

  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
  set(FETCHCONTENT_QUIET OFF)
  set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/deps")

  # SYSTEM is used to prevent applying compile flags to the dependencies
  # Do not build unnecessary SFML modules
  set(SFML_BUILD_NETWORK OFF)
  # Prevent SFML v3.0.1 from using the system locale (C locale when using VSCode on macOS)
  # if(NOT DEFINED ENV{LC_ALL})
  set(ENV{LC_ALL} "en_US.UTF-8")  # Always set, so it will work the first time instead of second time
  # endif()
  FetchContent_Declare(
    sfml
    #URL https://github.com/SFML/SFML/releases/download/3.0.0/SFML-3.0.0-sources.zip
    URL https://github.com/SFML/SFML/archive/refs/tags/3.0.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(sfml)

  FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(imgui)

  FetchContent_Declare(
    imgui-sfml
    URL https://github.com/SFML/imgui-sfml/archive/refs/tags/v3.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  set(IMGUI_DIR ${imgui_SOURCE_DIR})  # Location of ImGui src
  set(IMGUI_SFML_FIND_SFML OFF)       # Don't call "find_package(SFML)"
  FetchContent_MakeAvailable(imgui-sfml)

  FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(spdlog)

  FetchContent_Declare(
    tomlplusplus
    URL https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(tomlplusplus)

  # Link dependencies to the target
  target_link_libraries(${target} PUBLIC
    ImGui-SFML::ImGui-SFML  # ImGui-SFML already includes both ImGui and SFML
    SFML::Audio             # SFML audio for sound effects and music
    spdlog::spdlog
    tomlplusplus::tomlplusplus
  )

  # Set compile-time log level based on the build type
  # If debug build, set the log level to debug, otherwise keep it default (info)
  target_compile_definitions(${target} PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
  )

  # Link SFML::Main for WIN32 targets to manage the WinMain entry point
  # This makes Windows use "main()" instead of "WinMain()", so we can use the same entry point for all platforms
  if(WIN32)
    target_link_libraries(${target} PUBLIC SFML::Main)
  endif()

  message(STATUS "Linked dependencies 'ImGui-SFML' and 'spdlog' to target '${target}'.")
endfunction()

# Download and link test dependencies (automated testing)
function(fetch_and_link_external_test_dependencies target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist. Cannot fetch and link test dependencies.")
  endif()

  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
  set(FETCHCONTENT_QUIET OFF)
  set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/deps")

  FetchContent_Declare(
    snitch
    URL https://github.com/snitch-org/snitch/archive/refs/tags/v1.3.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(snitch)

  # Link test dependencies to target
  target_link_libraries(${target} PRIVATE snitch::snitch)

  message(STATUS "Linked test dependency 'snitch' to target '${target}'.")
endfunction()
