#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <cwchar>

#include "platform.hpp"

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
    #if PLATFORM_WINDOWS
        using FileDescriptorType = void*;
    #else
        using FileDescriptorType = int;
    #endif

        FileDescriptorType fd[2] = {0, 0};

    public:
        Pipe();

        auto getEnd(PipeSide side) const -> FileDescriptorType;

        auto closeEnd(PipeSide side) const -> void;

        auto duplicateEndTo(PipeSide side, int fd, bool closeAfterDuplicate = false) const -> void;

        auto readChunkOfData(std::string& to, char buffer[], std::size_t bufferSize) const -> int64_t;
    };

    template<typename T>
    concept CONVERTIBLE_TO_C_STRING =
        std::is_convertible_v<T, const char*> ||
        std::is_same_v<decltype(std::declval<T>().c_str()), const char*> ||
        std::is_same_v<decltype(std::declval<T>().c_str()), const wchar_t*>;

    template<typename T>
    constexpr auto deduceCString(const T& str) -> const char*
    {
        if constexpr (std::is_convertible_v<T, const char*>)
        {
            return str;
        }
        else if constexpr (std::is_same_v<decltype(str.c_str()), const wchar_t*>)
        {
            const wchar_t* input = str.c_str();

            std::mbstate_t state     = std::mbstate_t();
            const std::size_t length = 1 + std::wcsrtombs(nullptr, &input, 0, &state);

            std::vector<char> mbstr(length);
            std::wcsrtombs(&mbstr[0], &input, length, &state);

            const char* data = mbstr.data();
            return data;
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
