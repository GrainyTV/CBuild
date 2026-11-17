#include "process.hpp"
#include "terminate.hpp"
#include "stringutils.hpp"

#define nameof(identifier) #identifier

Process::Pipe::Pipe()
{
    if (pipe(fd) != 0)
    {
        Terminate::posixError(nameof(pipe));
    }
}

auto Process::Pipe::getEnd(PipeSide side) const -> int
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
    if (close(Pipe::getEnd(side)) != 0)
    {
        Terminate::posixError(nameof(close));
    }
}

auto Process::Pipe::duplicateEndTo(PipeSide side, int fd, bool closeAfterDuplicate) const -> void
{
    if (dup2(Pipe::getEnd(side), fd) < 0)
    {
        Terminate::posixError(nameof(dup2));
    }

    if (closeAfterDuplicate)
    {
        closeEnd(side);
    }
}

auto Process::Pipe::readChunkOfData(std::string& to, char buffer[], std::size_t bufferSize) const -> std::size_t
{
    std::size_t readAmount = read(Pipe::getEnd(PipeSide::READ_END), buffer, bufferSize);

    if (readAmount < 0)
    {
        Terminate::posixError(nameof(read));
    }
    else if (readAmount > 0)
    {
        to.append(buffer, readAmount);
    }

    return readAmount;
}

auto Process::run(const Process::Input& input) -> Process::Result
{
    Pipe opipe, epipe;

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
    }

    // Original Parent Process Branch
    // ---
    opipe.closeEnd(PipeSide::WRITE_END);
    epipe.closeEnd(PipeSide::WRITE_END);

    std::string out;
    std::string err;

    char buffer[4096];
    int status;

    std::size_t readAmount = -1;

    while (readAmount != 0)
    {
        readAmount = 0;
        readAmount += opipe.readChunkOfData(out, buffer, sizeof(buffer));
        readAmount += epipe.readChunkOfData(err, buffer, sizeof(buffer));
    }

    opipe.closeEnd(PipeSide::READ_END);
    epipe.closeEnd(PipeSide::READ_END);

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
}
