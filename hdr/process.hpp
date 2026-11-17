#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#ifdef __unix__
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace Process
{
    using Input = std::string;

    struct Result
    {
        std::string output;
        std::string error;
        int exitcode;
    };

    enum class PipeSide : std::uint8_t
    {
        WRITE_END,
        READ_END,
    };

    class Pipe
    {
    private:
        int fd[2] = {0, 0};

    public:
        Pipe();

        auto getEnd(PipeSide side) const -> int;

        auto closeEnd(PipeSide side) const -> void;

        auto duplicateEndTo(PipeSide side, int fd, bool closeAfterDuplicate = false) const -> void;

        auto readChunkOfData(std::string& to, char buffer[], std::size_t bufferSize) const -> std::size_t;
    };

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
    auto create(Executable&& executable, Args&& ...args) -> Process::Input
    {
        Process::Input input{deduceCString(std::forward<Executable>(executable))};
        ((input += ' ' + std::string{deduceCString(std::forward<Args>(args))}), ...);

        return input;
    }

    auto run(const Process::Input& input) -> Process::Result;
}
