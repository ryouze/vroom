include(FetchContent)

function(fetch_and_link_external_dependencies target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Target '${target}' does not exist. Cannot fetch and link dependencies.")
  endif()

  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
  set(FETCHCONTENT_QUIET OFF)
  set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/deps")

  # SYSTEM is used to prevent applying compile flags to the dependencies
  FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/11.1.4/fmt-11.1.4.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(fmt)

  # Do not build unnecessary SFML modules
  set(SFML_BUILD_AUDIO OFF)
  set(SFML_BUILD_NETWORK OFF)
  FetchContent_Declare(
    sfml
    URL https://github.com/SFML/SFML/releases/download/3.0.0/SFML-3.0.0-sources.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(sfml)

  FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(imgui)

  FetchContent_Declare(
    imgui-sfml
    URL https://github.com/SFML/imgui-sfml/archive/refs/tags/v3.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  set(IMGUI_DIR ${imgui_SOURCE_DIR})  # Location of ImGui src
  set(IMGUI_SFML_FIND_SFML OFF)       # Don't call "find_package(SFML)"
  set(IMGUI_SFML_IMGUI_DEMO ON)       # DEBUG, REMOVE LATER
  FetchContent_MakeAvailable(imgui-sfml)

  # Link dependencies to the target
  target_link_libraries(${target} PUBLIC fmt::fmt ImGui-SFML::ImGui-SFML)  # ImGui-SFML already includes both ImGui and SFML

  # Link SFML::Main for WIN32 targets to manage the WinMain entry point
  # This makes Windows use main() instead of WinMain(), so we can use the same entry point for all platforms
  if(WIN32)
    target_link_libraries(${target} PUBLIC SFML::Main)
  endif()

  message(STATUS "Linked dependencies 'fmt' and 'ImGui-SFML' to target '${target}'.")
endfunction()
