//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

static auto CopyFileFunc = &::CopyFileW;
static auto CopyFile2Func = [](PCWSTR from, PCWSTR to, bool failIfExists)
{
	BOOL cancel = false;
	COPYFILE2_EXTENDED_PARAMETERS params = { sizeof(params) };
	params.dwCopyFlags = failIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0;
	params.pfCancel = &cancel;

	auto result = ::CopyFile2(from, to, &params);
	if (HRESULT_FACILITY(result) == FACILITY_WIN32)
	{
		::SetLastError(result & 0x0000FFFF);
	}
	else
	{
		// Not a Win32 error, but we should be able to differentiate based on value
		::SetLastError(result);
	}

	return SUCCEEDED(result);
};
static auto CopyFileExFunc = [](PCWSTR from, PCWSTR to, bool failIfExists)
{
	return CopyFileExW(from, to, nullptr, nullptr, nullptr, failIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0);
};

static int DoCopyFileTest(
	const std::function<BOOL(PCWSTR, PCWSTR, bool)>& copyFn,
	const char* copyFnName,
	const std::filesystem::path& from,
	const std::filesystem::path& to,
	const char* expectedContents)
{
	trace_messages(L"Copying file from: ", info_color, from.native(), new_line);
	trace_messages(L"               to: ", info_color, to.native(), new_line);
	trace_messages(L"   Using function: ", info_color, copyFnName, new_line);

	// The initial copy should succeed, even when 'failIfExists' is true
	if (!copyFn(from.c_str(), to.c_str(), true))
	{
		return trace_last_error(L"Initial copy of the file failed");
	}

	auto contents = read_entire_file(to.c_str());
	trace_messages(L"Copied file contents: ", info_color, contents.c_str(), new_line);
	if (contents != expectedContents)
	{
		trace_message(L"ERROR: Copied file contents did not match the expected contents\n", error_color);
		trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, expectedContents, new_line);
		return ERROR_ASSERTION_FAILURE;
	}

	// Trying to copy again with 'failIfExists' as true should fail
	trace_message(L"Copying the file again to ensure that the copy fails due to the destination already existing...\n");
	if (copyFn(from.c_str(), to.c_str(), true))
	{
		trace_message(L"ERROR: Second copy succeeded even though 'failIfExists' was true\n", error_color);
		return ERROR_ASSERTION_FAILURE;
	}
	else if (::GetLastError() != ERROR_FILE_EXISTS)
	{
		return trace_last_error(L"The second copy failed as expected, but the error was not set to ERROR_FILE_EXISTS");
	}

	// Change the source's file contents so that we can validate that the copy truly did succeed
	// NOTE: This will also validate that we properly choose between a package file and its redirected equivalent
	expectedContents = "You are reading the modified text";
	write_entire_file(from.c_str(), expectedContents);

	// Trying to copy again with 'failIfExists' as false _should_ succeed this time
	trace_message(L"Copying the file a third time, but this time with 'failIfExists' as false...\n");
	if (!copyFn(from.c_str(), to.c_str(), false))
	{
		return trace_last_error(L"The third copy of the file failed, even though 'failIfExists' was false");
	}

	contents = read_entire_file(to.c_str());
	trace_messages(L"Copied file contents: ", info_color, contents.c_str(), new_line);
	if (contents != expectedContents)
	{
		trace_message(L"ERROR: Copied file contents did not match the expected contents\n", error_color);
		trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, expectedContents, new_line);
		return ERROR_ASSERTION_FAILURE;
	}

	return ERROR_SUCCESS;
}

inline int TestPathRedirectionLogic(std::filesystem::path fileFileSystemPath)
{
	auto file = ::CreateFileW(fileFileSystemPath.native().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	auto bufferSize = GetFinalPathNameByHandleW(file, nullptr, 0, 0);

	if (0 == bufferSize)
	{
		return trace_last_error(L"Error when getting the buffer size for the file path");
	}

	auto fileStringBuffer = std::make_unique<TCHAR[]>(bufferSize);
	bufferSize = GetFinalPathNameByHandleW(file, fileStringBuffer.get(), bufferSize, 0);

	if (0 == bufferSize)
	{
		return trace_last_error(L"Error when getting the file path");
	}

	std::wstring packageFamilyName = psf::current_package_family_name();
	std::wstring filePath(fileStringBuffer.get());
	if (filePath.find(packageFamilyName) == std::wstring::npos)
	{
		trace_message(L"ERROR: written file does not contain the package family name\n", error_color);
		trace_message(L"ERROR: File written to " + filePath, error_color);
        ::DeleteFileW(fileFileSystemPath.native().c_str());
		return ERROR_ASSERTION_FAILURE;
	}

    fileStringBuffer.release();
    ::DeleteFileW(fileFileSystemPath.native().c_str());
    ::CloseHandle(file);
	return ERROR_SUCCESS;
}

int CopyFileTests()
{
	int result = ERROR_SUCCESS;

	auto packageFilePath = g_packageRootPath / g_packageFileName;
	auto otherFilePath = g_packageRootPath / L"Çôƥ¥Fïℓè.txt";

	// Copy from a file in the package path
	test_begin("CopyFile From Package Test");
	clean_redirection_path();
	auto testResult = DoCopyFileTest(CopyFileFunc, "CopyFile", packageFilePath, otherFilePath, g_packageFileContents);
	result = result ? result : testResult;
	test_end(testResult);

	test_begin("CopyFile2 From Package Test");
	clean_redirection_path();
	testResult = DoCopyFileTest(CopyFile2Func, "CopyFile2", packageFilePath, otherFilePath, g_packageFileContents);
	result = result ? result : testResult;
	test_end(testResult);

	test_begin("CopyFileEx From Package Test");
	clean_redirection_path();
	testResult = DoCopyFileTest(CopyFileExFunc, "CopyFileEx", packageFilePath, otherFilePath, g_packageFileContents);
	result = result ? result : testResult;
	test_end(testResult);

	// Copy from a created file to a file in the package path
	// NOTE: Ideally the first copy with 'failIfExists' set to true would fail, but due to the limitations around file
	//       deletion, we explicitly ensure that the call won't fail. If this is ever changes, this test will start to
	//       fail, at which point we'll need to update this assumption
	const char otherFileContents[] = "You are reading from the generated file";

	test_begin("CopyFile To Package Test");
	clean_redirection_path();
	write_entire_file(otherFilePath.c_str(), otherFileContents);
	testResult = DoCopyFileTest(CopyFileFunc, "CopyFile", otherFilePath, packageFilePath, otherFileContents);
	result = result ? result : testResult;
	test_end(testResult);

	test_begin("CopyFile2 To Package Test");
	clean_redirection_path();
	write_entire_file(otherFilePath.c_str(), otherFileContents);
	testResult = DoCopyFileTest(CopyFile2Func, "CopyFile2", otherFilePath, packageFilePath, otherFileContents);
	result = result ? result : testResult;
	test_end(testResult);

	test_begin("CopyFileEx To Package Test");
	clean_redirection_path();
	write_entire_file(otherFilePath.c_str(), otherFileContents);
	testResult = DoCopyFileTest(CopyFileExFunc, "CopyFileEx", otherFilePath, packageFilePath, otherFileContents);
	result = result ? result : testResult;
	test_end(testResult);

    //Following tests do not work at the moment.
    //Pushing them to git so I won't lose my work.
	auto nonPathDirectory = std::filesystem::path(L"C:\\Program Files\\WindowsApps\\" + psf::current_package_full_name() + L"\\VFS\\SystemX86");
	std::filesystem::create_directory(nonPathDirectory);
	
	auto packageFilePathInRoot = g_packageRootPath / g_packageFileName;
    auto packageFilePathNotInRoot = nonPathDirectory / std::filesystem::path(L"InitialFile.txt");


	test_begin("CopyFile To non-package root");
	clean_redirection_path();
	int inRootResult = TestPathRedirectionLogic(packageFilePathInRoot);
	result = result ? result : inRootResult;
	test_end(inRootResult);

    
	test_begin("CopyFile To non-package root");
	clean_redirection_path();
	int notInRootResult = TestPathRedirectionLogic(packageFilePathNotInRoot);
	result = result ? result : notInRootResult;
	test_end(notInRootResult);

	return result;
}
