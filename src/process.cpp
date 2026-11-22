#include "process.hpp"
#include "terminate.hpp"

#include <cstdlib>

#if PLATFORM_WINDOWS
    #include <windows.h>

#elif PLATFORM_LINUX
    #include <poll.h>
    #include <sys/wait.h>
    #include <unistd.h>

#endif

#define nameof(identifier) #identifier

Process::Pipe::Pipe()
{
#if PLATFORM_WINDOWS
    SECURITY_ATTRIBUTES attrs;
    attrs.nLength              = sizeof(SECURITY_ATTRIBUTES);
    attrs.bInheritHandle       = true;
    attrs.lpSecurityDescriptor = nullptr;

    if (CreatePipe(&fd[0], &fd[1], &attrs, 4096) == 0)
    {
        Terminate::winapiError(nameof(CreatePipe));
    }

    // Only allow write end to be inherited [R|No] <===> [W|Yes]
    // We don't want to read in child process
    // ---
    if (SetHandleInformation(fd[0], HANDLE_FLAG_INHERIT, false) == 0)
    {
        Terminate::winapiError(nameof(SetHandleInformation));
    }

#else
    if (pipe(fd) != 0)
    {
        Terminate::posixError(nameof(pipe));
    }

#endif
}

auto Process::Pipe::getEnd(PipeSide side) const -> FileDescriptorType
{
    switch (side)
    {
        case PipeSide::READ_END:
            return fd[0];
            break;

        case PipeSide::WRITE_END:
            return fd[1];
            break;
    }
}

auto Process::Pipe::closeEnd(PipeSide side) const -> void
{
#if PLATFORM_WINDOWS
    if (CloseHandle(Pipe::getEnd(side)) == 0)
    {
        Terminate::winapiError(nameof(CloseHandle));
    }

#else
    if (close(Pipe::getEnd(side)) != 0)
    {
        Terminate::posixError(nameof(close));
    }

#endif
}

auto Process::Pipe::duplicateEndTo(PipeSide side, int fd, bool closeAfterDuplicate) const -> void
{
#if PLATFORM_WINDOWS
#else
    if (dup2(Pipe::getEnd(side), fd) < 0)
    {
        Terminate::posixError(nameof(dup2));
    }

#endif

    if (closeAfterDuplicate)
    {
        closeEnd(side);
    }
}

auto Process::Pipe::readChunkOfData(std::string& to, char buffer[], std::size_t bufferSize) const -> int64_t
{
#if PLATFORM_WINDOWS
    return 1;

#else
    ssize_t readAmount = read(Pipe::getEnd(PipeSide::READ_END), buffer, bufferSize);

    if (readAmount < 0)
    {
        Terminate::posixError(nameof(read));
    }
    else if (readAmount > 0)
    {
        to.append(buffer, readAmount);
    }

    return readAmount;

#endif
}

auto Process::run(const Process::Input& input) -> Process::Result
{
    Pipe opipe;
    Pipe epipe;

#if PLATFORM_WINDOWS
    STARTUPINFO st;
    ZeroMemory(&st, sizeof(STARTUPINFO));
    st.cb         = sizeof(STARTUPINFO);
    st.hStdOutput = opipe.getEnd(PipeSide::WRITE_END);
    st.hStdError  = epipe.getEnd(PipeSide::WRITE_END);
    st.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    st.dwFlags   |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    // This is only needed for barebones alpha testing using WINE
    // It should be removed very soon ...
    // ---
    SetEnvironmentVariable("PATH",
        std::format("{:s};{:s};{:s};{:s}",
            std::getenv("PATH"),
            "Z:\\home\\akane\\Documents\\Apps\\mingw-windows\\bin",
            "Z:\\home\\akane\\Documents\\Apps\\mingw-windows\\libexec\\gcc\\x86_64-w64-mingw32\\16.2.0",
            "Z:\\usr\\bin"
        ).c_str()
    );

    // SetEnvironmentVariable("PATHEXT",
    //     std::format("{:s};{:s}",
    //         std::getenv("PATHEXT"), "."
    //     ).c_str()
    // );

    bool processStarted = CreateProcess(
        nullptr,                          // Module name UNUSED (input already contains it)
        const_cast<char*>(input.c_str()), // The actual command invoked
        nullptr,
        nullptr,
        true,                             // Inherit standard handles
        0,
        nullptr,
        nullptr,
        &st,
        &pi
    );

    if (not processStarted)
    {
        Terminate::winapiError(nameof(CreateProcess));
    }

    // Child started properly, back to Parent process
    // ---
    opipe.closeEnd(PipeSide::WRITE_END);
    epipe.closeEnd(PipeSide::WRITE_END);

    // fmt::println("Process handle: {}", (void*)pi.hProcess);
    // fmt::println("Thread handle: {}", (void*)pi.hThread);
    // fmt::println("Process ID: {}", pi.dwProcessId);

    std::string out;
    std::string err;

    char buffer[4096];
    DWORD readAmount  = 0;
    DWORD exitcode    = 0;
    bool pipeFinished = false;

    while (not pipeFinished)
    {
        if (not ReadFile(opipe.getEnd(PipeSide::READ_END), buffer, 4096, &readAmount, nullptr))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                pipeFinished = true;
                continue;
            }

            Terminate::winapiError(nameof(ReadFile));
        }

        if (readAmount == 0)
        {
            pipeFinished = true;
            continue;
        }

        out.append(buffer, readAmount);
    }

    pipeFinished = false;

    while (not pipeFinished)
    {
        if (not ReadFile(epipe.getEnd(PipeSide::READ_END), buffer, 4096, &readAmount, nullptr))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                pipeFinished = true;
                continue;
            }

            Terminate::winapiError(nameof(ReadFile));
        }

        if (readAmount == 0)
        {
            pipeFinished = true;
            continue;
        }

        err.append(buffer, readAmount);
    }

    opipe.closeEnd(PipeSide::READ_END);
    epipe.closeEnd(PipeSide::READ_END);

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
    {
        Terminate::winapiError(nameof(WaitForSingleObject));
    }

    if (not GetExitCodeProcess(pi.hProcess, &exitcode))
    {
        Terminate::winapiError(nameof(GetExitCodeProcess));
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return {
        .output   = String::rstrip(std::move(out)),
        .error    = String::rstrip(std::move(err)),
        .exitcode = static_cast<int>(exitcode),
    };

#else
    int childPid = fork();

    if (childPid < 0)
    {
        Terminate::posixError(nameof(fork));
    }

    if (childPid == 0)
    {
        // Forked Child Process Branch
        // ---
        opipe.closeEnd(PipeSide::READ_END);
        epipe.closeEnd(PipeSide::READ_END);

        opipe.duplicateEndTo(PipeSide::WRITE_END, STDOUT_FILENO, true);
        epipe.duplicateEndTo(PipeSide::WRITE_END, STDERR_FILENO, true);

        execlp("sh", "sh", "-c", input.c_str(), nullptr);
        std::exit(1);
    }

    // Original Parent Process Branch
    // ---
    opipe.closeEnd(PipeSide::WRITE_END);
    epipe.closeEnd(PipeSide::WRITE_END);

    pollfd readPipes[] = {
        { opipe.getEnd(PipeSide::READ_END), POLLIN, 0 },
        { epipe.getEnd(PipeSide::READ_END), POLLIN, 0 },
    };

    std::string out;
    std::string err;

    char buffer[4096];
    int status;

    const std::size_t pipeCount = 2;
    std::size_t openPipes       = pipeCount;

    while (openPipes > 0)
    {
        int pipesReady = poll(readPipes, pipeCount, 250);

        if (pipesReady < 0)
        {
            Terminate::posixError(nameof(poll));
        }

        if (pipesReady == 0)
        {
            // Timeout occurred
            // ---
            continue;
        }

        int outReturn = readPipes[0].revents;
        int errReturn = readPipes[1].revents;

        if (outReturn != 0)
        {
            // fmt::println("STDOUT({:d}={:d}|{:d}|{:d})", outReturn, POLLIN, POLLERR, POLLHUP);

            if ((outReturn & POLLIN) != 0)
            {
                opipe.readChunkOfData(out, buffer, sizeof(buffer));
            }
            else
            {
                opipe.closeEnd(PipeSide::READ_END);
                openPipes -= 1;
            }
        }

        if (errReturn != 0)
        {
            // fmt::println("STDERR({:d}={:d}|{:d}|{:d})", errReturn, POLLIN, POLLERR, POLLHUP);

            if ((errReturn & POLLIN) != 0)
            {
                epipe.readChunkOfData(err, buffer, sizeof(buffer));
            }
            else
            {
                epipe.closeEnd(PipeSide::READ_END);
                openPipes -= 1;
            }
        }
    }

    if (waitpid(childPid, &status, 0) < 0)
    {
        Terminate::posixError(nameof(waitpid));
    }

    const bool exited = WIFEXITED(status);

    return {
        .output   = String::rstrip(std::move(out)),
        .error    = String::rstrip(std::move(err)),
        .exitcode = exited ? WEXITSTATUS(status) : 1,
    };

#endif
}
