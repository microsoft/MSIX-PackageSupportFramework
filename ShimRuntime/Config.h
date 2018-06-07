#pragma once

#include <filesystem>
#include <string>

void LoadConfig();

// Globals set by `LoadConfig`, to avoid continuously querying them
const std::wstring& PackageFullName() noexcept;
const std::wstring& ApplicationUserModelId() noexcept;
const std::wstring& ApplicationId() noexcept;
const std::filesystem::path& PackageRootPath() noexcept;
