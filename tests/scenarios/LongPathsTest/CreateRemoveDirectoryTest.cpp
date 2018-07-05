
#include <test_config.h>

#include "paths.h"

static int DoCreateRemoveDirectoryTest()
{
    clean_redirection_path();

    auto path = g_testPath / L"Foo";
    trace_messages(L"Creating directory: ", info_color, path.native(), new_line);

    if (!::CreateDirectoryW(path.c_str(), nullptr))
    {
        return trace_last_error(L"CreateDirectory failed");
    }

    trace_message(L"Directory created successfully... Now trying to remove it\n");
    if (!::RemoveDirectoryW(path.c_str()))
    {
        return trace_last_error(L"RemoveDirectory failed");
    }
    else if (std::filesystem::exists(path))
    {
        trace_message(L"ERROR: RemoveDirectory succeeded, but the directory still exists\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int CreateRemoveDirectoryTest()
{
    test_begin("Create/Remove Directory Test");
    auto result = DoCreateRemoveDirectoryTest();
    test_end(result);
    return result;
}
