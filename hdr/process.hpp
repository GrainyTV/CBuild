#pragma once
#include <string>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef __unix__
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace Process
{
    struct Result
    {
        std::string output;
        std::string error;

        int exitcode;
    };

    class Pipe
    {
    private:
        int fd[2] = {0, 0};

        auto closeEnd(int end) -> void
        {
            if (close(end) != 0)
            {
                std::cout << "close failed\n";
            }
        }

        auto duplicateTo(int end, int fd, bool closeAfterDuplicate) -> void
        {
            if (dup2(end, fd) < 0)
            {
                std::cout << "dup2 failed\n";
            }

            if (closeAfterDuplicate)
            {
                closeEnd(end);
            }
        }

    public:
        Pipe()
        {
            if (pipe(fd) != 0)
            {
                std::cout << "pipe failed\n";
            }
        }

        auto readEnd() -> int
        {
            return fd[0];
        }

        auto writeEnd() -> int
        {
            return fd[1];
        }

        auto duplicateReadEndTo(int fd, bool closeAfterDuplicate = false) -> void
        {
            duplicateTo(readEnd(), fd, closeAfterDuplicate);
        }

        auto duplicateWriteEndTo(int fd, bool closeAfterDuplicate = false) -> void
        {
            duplicateTo(writeEnd(), fd, closeAfterDuplicate);
        }

        auto closeReadEnd() -> void
        {
            closeEnd(readEnd());
        }

        auto closeWriteEnd() -> void
        {
            closeEnd(writeEnd());
        }
    };

    inline auto rstrip(std::string& str) -> std::string
    {
        if (not str.empty() && str.ends_with('\n'))
        {
            return str.erase(str.size() - 1);
        }

        return str;
    }

    template<typename T>
    concept CONVERTIBLE_TO_C_STRING =
        std::is_convertible_v<T, const char*> ||
        std::is_same_v<decltype(std::declval<T>().c_str()), const char*>;

    template<typename T>
    constexpr auto deduceCString(const T& str) -> const char*
    {
        if constexpr (std::is_convertible_v<T, const char*>)
        {
            return str;
        }
        else
        {
            return str.c_str();
        }
    }

    template<CONVERTIBLE_TO_C_STRING Executable, CONVERTIBLE_TO_C_STRING... Args>
    auto run(Executable&& executable, Args&& ...args) -> Process::Result
    {
        Pipe opipe;
        Pipe epipe;

        int childPid = fork();

        if (childPid < 0)
        {
            std::cout << "fork failed\n";
        }

        if (childPid == 0)
        {
            // Forked Child Process Branch
            // ---
            opipe.closeReadEnd();
            epipe.closeReadEnd();

            opipe.duplicateWriteEndTo(STDOUT_FILENO, true);
            epipe.duplicateWriteEndTo(STDERR_FILENO, true);

            std::string cmd{deduceCString(executable)};
            ((cmd += " " + std::string{deduceCString(std::forward<Args>(args))}), ...);

            execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        }

        // Original Parent Process Branch
        // ---
        opipe.closeWriteEnd();
        epipe.closeWriteEnd();

        std::string out;
        std::string err;

        char buf[4096];
        int status;

        while (true)
        {
            size_t oamount = read(opipe.readEnd(), buf, sizeof(buf));

            if (oamount > 0)
            {
                out.append(buf, oamount);
            }

            size_t eamount = read(epipe.readEnd(), buf, sizeof(buf));

            if (eamount > 0)
            {
                err.append(buf, eamount);
            }

            if (oamount + eamount == 0)
            {
                break;
            }
        }

        opipe.closeReadEnd();
        epipe.closeReadEnd();

        if (waitpid(childPid, &status, 0) < 0)
        {
            std::cout << "waitpid failed\n";
        }

        const bool exited = WIFEXITED(status);

        return {
            .output   = rstrip(out),
            .error    = rstrip(err),
            .exitcode = exited ? WEXITSTATUS(status) : 1,
        };
    }
}
