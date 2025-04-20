/**
 * @file main.cpp
 */

#include <cstdlib>    // for EXIT_FAILURE, EXIT_SUCCESS
#include <exception>  // for std::exception

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>         // for SetConsoleCP, SetConsoleOutputCP, CP_UTF8
#endif

#include <spdlog/spdlog.h>

#include "app.hpp"
#include "generated.hpp"

/**
 * @brief Entry-point of the application.
 *
 * This sets up basic boilerplate, then calls "app::run()" to start the application.
 *
 * @return EXIT_SUCCESS if the application ran successfully, EXIT_FAILURE otherwise.
 */
int main()
{
    try {
        // Set compile-time log level based on the build type
        // If debug build, set the log level to debug, otherwise keep it default (info)
        // Note: Although CMake also sets the "SPDLOG_ACTIVE_LEVEL" level to "SPDLOG_LEVEL_DEBUG" for debug builds, "spdlog::set_level" is still needed to set the log level for the current process
#ifndef NDEBUG  // "Not Debug" build (Release)
        spdlog::set_level(spdlog::level::debug);
#endif
        // Log low-level debug information
        SPDLOG_DEBUG("Build - Version: {}, Config: {}, Date: {}, Time: {}",
                     generated::PROJECT_VERSION,
                     generated::BUILD_CONFIGURATION,
                     generated::BUILD_DATE,
                     generated::BUILD_TIME);

        SPDLOG_DEBUG("Compiler - {}, C++ standard: {}",
                     generated::COMPILER_INFO,
                     generated::CPP_STANDARD);

        SPDLOG_DEBUG("Platform - OS: {} ({}), Shared Libs: {}, Strip: {}, LTO: {}",
                     generated::OPERATING_SYSTEM,
                     generated::ARCHITECTURE,
                     generated::BUILD_SHARED_LIBS,
                     generated::STRIP_ENABLED,
                     generated::LTO_ENABLED);

        SPDLOG_DEBUG("Logging - Level: {}",
                     spdlog::level::to_string_view(spdlog::get_level()));

#if defined(_WIN32)  // Setup UTF-8 input/output
        SPDLOG_DEBUG("Windows platform detected, setting console to UTF-8...");
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        SPDLOG_DEBUG("Set console to UTF-8!");
#endif
        // Call the application entry point
        SPDLOG_INFO("Starting application...");
        app::run();
    }
    catch (const std::exception &e) {
        SPDLOG_CRITICAL("{}", e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        SPDLOG_CRITICAL("Unknown error occurred!");
        return EXIT_FAILURE;
    }

    // Normal exit
    SPDLOG_DEBUG("Application exited normally!");
    return EXIT_SUCCESS;
}
