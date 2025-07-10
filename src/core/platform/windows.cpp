/**
 * @file windows.cpp
 */

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

#include <filesystem>  // for std::filesystem
#include <memory>      // for std::unique_ptr
#include <stdexcept>   // for std::runtime_error

#include <combaseapi.h>  // for CoTaskMemFree
#include <shlobj.h>      // for SHGetKnownFolderPath, FOLDERID_LocalAppData, KF_FLAG_*
#include <windows.h>     // for HRESULT, CoInitializeEx, CoUninitialize, COINIT_*

#include <spdlog/spdlog.h>

#include "windows.hpp"

namespace core::platform::windows {

// std::filesystem::path get_local_appdata_directory()
// {
//     SPDLOG_DEBUG("Retrieving LocalAppData directory using Windows API without COM...");
//     // RAII wrapper for the COM-allocated buffer
//     constexpr auto deleter = [](const PWSTR p) noexcept { if (p) CoTaskMemFree(p); };  // Will work even if p is nullptr, but why not
//     using unique_pwstr = std::unique_ptr<wchar_t, decltype(deleter)>;

//     PWSTR raw_path;
//     // "When this method returns, contains the address of a pointer to a null-terminated Unicode string that specifies the path of the known folder. The calling process is responsible for freeing this resource once it is no longer needed by calling CoTaskMemFree, whether SHGetKnownFolderPath succeeds or not."
//     // https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
//     const HRESULT hresult_code = SHGetKnownFolderPath(
//         FOLDERID_LocalAppData,  // %LOCALAPPDATA%
//         KF_FLAG_DEFAULT,        // Do not create the directory
//         nullptr,                // Current user
//         &raw_path);             // Out: pointer to wide‐string
//     SPDLOG_DEBUG("SHGetKnownFolderPath called, now creating RAII guard...");

//     unique_pwstr holder{raw_path, deleter};  // Now owns raw_path
//     SPDLOG_DEBUG("RAII guard created, checking for errors...");

//     if (FAILED(hresult_code)) [[unlikely]] {
//         // Keep error in decimal like "set_titlebar_icon" for consistency
//         throw std::runtime_error(std::format("Failed to get the LocalAppData directory on Windows: {}", static_cast<unsigned long>(hresult_code)));
//     }
//     SPDLOG_DEBUG("No errors, converting to std::filesystem::path...");

//     const std::filesystem::path result{holder.get()};
//     SPDLOG_DEBUG("LocalAppData directory successfully retrieved as '{}', returning it!", result.string());  // Debug only, we don't care about UTF-16 issues
//     return result;
// }

std::filesystem::path get_local_appdata_directory()
{
    SPDLOG_DEBUG("Retrieving LocalAppData directory using Windows API with COM...");

    // Need COM to use SHGetKnownFolderPath
    SPDLOG_DEBUG("Initializing COM for Known Folders...");
    if (const HRESULT com_init_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        // S_OK means success, S_FALSE means already initialized
        com_init_result != S_OK && com_init_result != S_FALSE) [[unlikely]] {
        throw std::runtime_error("Failed to initialize COM for Windows Known Folders API");
    }
    SPDLOG_DEBUG("COM initialized successfully, now retrieving LocalAppData path...");

    PWSTR raw_path = nullptr;

    // "When this method returns, contains the address of a pointer to a null-terminated Unicode string that specifies the path of the known folder. The calling process is responsible for freeing this resource once it is no longer needed by calling CoTaskMemFree, whether SHGetKnownFolderPath succeeds or not."
    // https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
    HRESULT folder_path_result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,  // %LOCALAPPDATA%
        KF_FLAG_DEFAULT,        // Do not create the directory
        nullptr,                // Current user
        &raw_path);             // Out: pointer to wide‐string
    SPDLOG_DEBUG("SHGetKnownFolderPath called, now creating RAII guard...");

    // RAII; CoTaskMemFree(nullptr) does nothing, so it's OK to pass nullptr
    auto holder = std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)>(raw_path, &CoTaskMemFree);
    SPDLOG_DEBUG("RAII guard created, checking for errors...");
    if (FAILED(folder_path_result) || raw_path == nullptr) [[unlikely]] {
        CoUninitialize();  // Can't be bothered with RAII for this
        throw std::runtime_error("Failed to get LocalAppData directory path using Windows API");
    }
    SPDLOG_DEBUG("No errors, converting to std::filesystem::path...");

    std::filesystem::path result(holder.get());
    CoUninitialize();  // Can't be bothered with RAII for this
    SPDLOG_DEBUG("LocalAppData directory successfully retrieved as '{}', returning it!", result.string());
    return result;
}

}  // namespace core::platform::windows

#endif
