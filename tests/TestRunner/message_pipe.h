//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <memory>

#include <error_logging.h>
#include <fancy_handle.h>
#include <test_runner.h>

enum class pipe_state
{
    disconnected,
    connecting,
    connected
};

class message_pipe
{
public:

    message_pipe()
    {
        m_buffer = std::make_unique<char[]>(m_bufferCapacity);
        m_pipeHandle.reset(::CreateNamedPipeW(
            test_runner_pipe_name,
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE,
            PIPE_UNLIMITED_INSTANCES,      // MaxInstances
            2048,   // OutBufferSize
            2048,   // InBufferSize
            0,      // DefaultTimeOut (defaults to 50ms)
            nullptr));
        if (!m_pipeHandle)
        {
            throw_win32(print_last_error("Failed to create named pipe"));
        }

        m_pipeEvent.reset(::CreateEventW(nullptr, true, false, nullptr));
        if (!m_pipeEvent)
        {
            throw_win32(print_last_error("Failed to create event"));
        }

        m_overlapped.hEvent = m_pipeEvent.get();

        InitiateConnection();
    }

    pipe_state state() const noexcept
    {
        return m_state;
    }

    HANDLE wait_handle() const noexcept
    {
        return m_pipeEvent.get();
    }

    template <typename Handler> // void(const test_message*)
    void on_signalled(Handler&& handler)
    {
        ::ResetEvent(m_pipeEvent.get());

        if (m_state == pipe_state::connecting)
        {
            m_state = pipe_state::connected;
            InitiateRead();
            return;
        }

        assert(m_state == pipe_state::connected);
        DWORD bytesRead;
        if (!::GetOverlappedResult(m_pipeHandle.get(), &m_overlapped, &bytesRead, false))
        {
            if (::GetLastError() != ERROR_BROKEN_PIPE)
            {
                throw_win32(print_last_error("Failed to read from pipe"));
            }

            // Failure was because of client disconnecting. Disconnect and start listening for a new client
            InitiateReconnect();
            return;
        }

        // We got signalled because we got more data
        m_bufferSize += bytesRead;
        auto msg = reinterpret_cast<const test_message*>(m_buffer.get());
        if ((m_bufferSize >= sizeof(test_message)) && (m_bufferSize >= static_cast<DWORD>(msg->size)))
        {
            while ((m_bufferSize >= sizeof(test_message)) && (m_bufferSize >= static_cast<DWORD>(msg->size)))
            {
                handler(msg);
                m_bufferSize -= msg->size;
                msg = advance(msg);
            }

            std::memmove(m_buffer.get(), msg, m_bufferSize);
        }

        InitiateRead();
    }

private:

    void InitiateConnection()
    {
        assert(m_state == pipe_state::disconnected);

        [[maybe_unused]] auto result = ::ConnectNamedPipe(m_pipeHandle.get(), &m_overlapped);
        assert(!result);

        switch (::GetLastError())
        {
        case ERROR_IO_PENDING:
            // Completing asynchronously
            m_state = pipe_state::connecting;
            break;

        case ERROR_PIPE_CONNECTED:
            // Client has already connected to the pipe
            m_state = pipe_state::connected;
            InitiateRead();
            break;

        default:
            throw_win32(print_last_error("Failed to wait for client to connect to pipe"));
        }
    }

    void InitiateRead()
    {
        assert(m_state == pipe_state::connected);

        auto readResult = ::ReadFile(
            m_pipeHandle.get(),
            m_buffer.get() + m_bufferSize,
            m_bufferCapacity - m_bufferSize,
            nullptr,
            &m_overlapped);
        if (!readResult)
        {
            switch (::GetLastError())
            {
            case ERROR_IO_PENDING:
                // This is the expected result. Nothing to do
                break;

            case ERROR_BROKEN_PIPE:
                // Pipe has been closed by the client. Open the pipe for use by other consumers
                InitiateReconnect();
                break;

            default:
                throw_win32(print_last_error("Failed reading data from pipe"));
            }
        }
        else
        {
            assert(HasOverlappedIoCompleted(&m_overlapped));
        }
    }

    void InitiateReconnect()
    {
        assert(m_state == pipe_state::connected);
        if (!::DisconnectNamedPipe(m_pipeHandle.get()))
        {
            throw_win32(print_last_error("Failed to disconnect the pipe from the client"));
        }

        m_state = pipe_state::disconnected;
        InitiateConnection();
    }

    unique_handle m_pipeHandle;
    unique_handle m_pipeEvent;
    OVERLAPPED m_overlapped = {};
    pipe_state m_state = pipe_state::disconnected;

    DWORD m_bufferCapacity = 2048;
    DWORD m_bufferSize = 0;
    std::unique_ptr<char[]> m_buffer;
};
