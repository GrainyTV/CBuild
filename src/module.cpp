#include "module.hpp"

#include <string_view>
#include <filesystem>
#include <fstream>
#include <vector>

#include "fmt-ext.hpp"
#include "process.hpp"
#include "seq/seq.hpp"
#include "terminate.hpp"

using TomlExt::getTomlProperty;
namespace fs = std::filesystem;

static auto parseSourceFiles(const toml::array& sourceNodes) -> std::vector<std::string>
{
    const auto sourceFiles =
        sourceNodes
        | Seq::map([](const toml::node& node) { return *node.value_exact<std::string>(); })
        | Seq::toVector();

    sourceFiles
    | Seq::iter([](const auto& src) {
            if (not fs::exists(src))
            {
                Terminate::missingFile(src);
            }
        });

    return sourceFiles;
}

static auto parseModuleType(std::string_view type) -> ModuleType
{
    if (type == "executable")
    {
        return ModuleType::EXECUTABLE;
    }
    else if (type == "dynamiclib")
    {
        return ModuleType::DYNAMIC_LIB;
    }
    else if (type == "staticlib")
    {
        return ModuleType::STATIC_LIB;
    }

    Terminate::invalidModuleType(type);
}

BuildModule::BuildModule(const toml::table& module, const BuildContext& context)
    : context(context)
    , sources(parseSourceFiles(getTomlProperty<toml::array>(module, "sources")))
    , cflags(getTomlProperty<toml::array>(module, "cflags"))
    , output(getTomlProperty<std::string>(module, "output"))
    , type(parseModuleType(getTomlProperty<std::string>(module, "type")))
{
}

auto BuildModule::build() const -> void
{
    std::vector<fs::path> objectFiles;
    objectFiles.reserve(sources.size());

    const std::string cflagsFlattened = TomlExt::flattenStringArray(cflags);

    for (const auto& src : sources)
    {
        const fs::path srcFullpath = context.sourceDir / src;
        const fs::path objFullpath = context.buildDir / "obj" / (src + ".o"); // TODO: use the platform complaint object file extension

        fs::create_directories(objFullpath.parent_path());

        Process::Result cmd = Process::run(context.compiler, cflagsFlattened, "-c", "-o", objFullpath, srcFullpath);

        if (cmd.exitcode != 0)
        {
            Terminate::failedProcess(cmd);
        }

        objectFiles.emplace_back(objFullpath);
    }

    const std::string objectFilesFlattened =
        objectFiles
        | Seq::reduce(std::string{}, [](const fs::path& obj, std::string& accum) {
                if (accum.empty())
                {
                    return accum + obj.string();
                }
                else
                {
                    return accum + ' ' + obj.string();
                }
            });

    // TODO: check whether we actually need linking with stdc++
    // TODO: collect object files for linking step and flatten them to single param
    // ---
    const fs::path outArtifact = context.buildDir / output;

    Process::Result cmdCrt1 = Process::run(context.compiler, "--print-file-name", "crt1.o");
    Process::Result cmdCrti = Process::run(context.compiler, "--print-file-name", "crti.o");
    Process::Result cmdCrtn = Process::run(context.compiler, "--print-file-name", "crtn.o");
    Process::Result cmdCrtb = Process::run(context.compiler, "--print-file-name", "crtbeginT.o");
    Process::Result cmdCrte = Process::run(context.compiler, "--print-file-name", "crtend.o");

    fs::path baseLibDir = fs::path{cmdCrt1.output}.parent_path();
    fs::path gccLibDir  = fs::path{cmdCrtb.output}.parent_path();

    Process::Result cmd = Process::run(
        context.linker,
        "-static",
        "-o", outArtifact,
        "-L", baseLibDir,
        "-L", gccLibDir,
        objectFilesFlattened,
        cmdCrt1.output,
        cmdCrti.output,
        cmdCrtn.output,
        cmdCrtb.output,
        cmdCrte.output,
        "-lstdc++",
        "--start-group", "-lc", "-lgcc", "-lgcc_eh", "--end-group"
    );

    if (cmd.exitcode != 0)
    {
        Terminate::failedProcess(cmd);
    }
}

auto BuildModule::generateCompileCommandsJson() const -> void
{
    fs::path sourceDir = fs::current_path();
    fs::path buildDir  = sourceDir / "out";

    fs::create_directory(buildDir);
    fs::path compileCommands = buildDir / "compile_commands.json";

    {
        std::ofstream fileHandle{compileCommands};

        if (not fileHandle.is_open())
        {
            FmtExt::println("Failed to open compile_commands.json for writing");
            std::abort();
        }

        constexpr auto indent      = "  ";
        const std::size_t objCount = sources.size();
        std::vector<std::string> args;

        fmt::println(fileHandle, "[");

        for (std::size_t i = 0; i < objCount; ++i)
        {
            const auto src = fs::path{".."} / sources[i];

            const auto flags =
                cflags
                | Seq::map([](const toml::node& node) { return *node.value_exact<std::string>(); })
                | Seq::toVector();

            args.clear();
            args.emplace_back(context.compiler);
            args.insert(args.end(), flags.begin(), flags.end());
            args.emplace_back("-c");
            args.emplace_back(src);

            if (i != 0)
            {
                fmt::println(fileHandle, ",");
            }

            // LINE #1
            // ---
            fmt::print(fileHandle, indent);
            fmt::print(fileHandle, "{{ ");
            fmt::println(fileHandle, "\"directory\": \"{:s}\",", buildDir.c_str());

            // LINE #2
            // ---
            fmt::print(fileHandle, indent);
            fmt::print(fileHandle, indent);
            fmt::println(fileHandle, "\"arguments\": {},", args);

            // LINE #3
            // ---
            fmt::print(fileHandle, indent);
            fmt::print(fileHandle, indent);
            fmt::print(fileHandle, "\"file\": \"{:s}\"", src.c_str());
            fmt::print(fileHandle, " }}");
        }

        fmt::println(fileHandle, "\n]");
    }
}
