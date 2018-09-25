//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <filesystem>

#include <file_paths.h>

constexpr wchar_t g_programFilesRelativeTestPath[] = LR"(this\is\a\moderately\long\path\and\will\turn\into\an\even\longer\path\when\we\take\into\account\the\fact\that\it\will\have\the\package\path\appended\to\it\when\the\app\is\installed\to\the\WindowsApps\directory)";

inline const auto g_programFilesPath = psf::known_folder(FOLDERID_ProgramFiles);
inline const auto g_testPath = g_programFilesPath / g_programFilesRelativeTestPath;
inline const auto g_testFilePath = g_testPath / L"file.txt";

constexpr char g_expectedFileContents[] = "You are reading from the package path";
