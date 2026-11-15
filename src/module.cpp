#include "module.hpp"

#include <string_view>
#include <filesystem>
#include <fstream>
#include <vector>

#include "fmt-ext.hpp"
#include "process.hpp"
#include "seq/seq.hpp"
#include "terminate.hpp"

using Terminate::TerminateConditionWhen;
using TomlExt::getTomlProperty;
namespace fs = std::filesystem;

static auto flattenStringEnumerable(IEnumerable<std::string> entries) -> std::string
{
    return
        entries
        | Seq::reduce(std::string{}, [](const auto& elem, std::string& accum) {
                if (accum.empty())
                {
                    return accum + elem;
                }
                else
                {
                    return accum + ' ' + elem;
                }
            });
}

static auto parseSourceFiles(const toml::array& sourceNodes, const BuildContext& context) -> std::vector<fs::path>
{
    auto convertNodeToSourceFile = [&context](const toml::node& node)
    {
        const fs::path srcUser{*node.value_exact<std::string>()};

        if (srcUser.is_absolute() && fs::is_regular_file(srcUser))
        {
            Terminate::withMessage(
                fmt::format(
                    R"(Source file {:s} is provided using an absolute path, which is generally not recommended)" "\n"
                    R"(Either make it relative to the project directory, or add "enable-abspath = true" to your project file)",
                    srcUser
                ),
                TerminateConditionWhen::AT_CONFIGURE
            );
        }

        const fs::path src = context.sourceDir / srcUser;

        if (not fs::exists(src) || not fs::is_regular_file(src))
        {
            Terminate::missingFile(src.string());
        }

        return src;
    };

    return
        sourceNodes
        | Seq::map(convertNodeToSourceFile)
        | Seq::toVector();
}

static auto parseHeaderDirs(const toml::array& headerNodes, const BuildContext& context) -> std::vector<fs::path>
{
    auto convertNodeToHeaderDir = [&context](const toml::node& node)
    {
        const fs::path hdrUser{*node.value_exact<std::string>()};

        if (hdrUser.is_absolute() && fs::is_directory(hdrUser))
        {
            Terminate::withMessage(
                fmt::format(
                    R"(Header directory {:s} is provided using an absolute path, which is generally not recommended)" "\n"
                    R"(Either make it relative to the project directory, or add "enable-abspath = true" to your project file)",
                    hdrUser
                ),
                TerminateConditionWhen::AT_CONFIGURE
            );
        }

        const fs::path hdr = context.sourceDir / hdrUser;

        if (not fs::exists(hdr) || not fs::is_directory(hdr))
        {
            Terminate::missingFile(hdr.string());
        }

        return hdr;
    };

    return
        headerNodes
        | Seq::map(convertNodeToHeaderDir)
        | Seq::toVector();
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

BuildModule::BuildModule(const std::string& moduleName, const toml::table& module, const BuildContext& context)
    : context(context)
    , sources(parseSourceFiles(getTomlProperty<toml::array>(module, "sources"), context))
    , headers(parseHeaderDirs(getTomlProperty<toml::array, TomlLookupManner::OPTIONAL>(module, "headers").value_or(toml::array{}), context))
    , cflags(getTomlProperty<toml::array>(module, "cflags"))
    , output(getTomlProperty<std::string, TomlLookupManner::OPTIONAL>(module, "output").value_or(moduleName))
    , type(parseModuleType(getTomlProperty<std::string>(module, "type")))
{
}

auto BuildModule::build() const -> void
{
    std::vector<fs::path> objectFiles;
    objectFiles.reserve(sources.size());

    const std::string cflagsFlattened = TomlExt::flattenStringArray(cflags);

    const std::string headerDirs =
        flattenStringEnumerable(headers | Seq::map([](const fs::path& hdr) { return "-I" + hdr.string(); }));

    for (const fs::path& src : sources)
    {
        fs::path srcRelative = fs::relative(src, context.sourceDir);
        const fs::path obj   = context.buildDir / "obj" / srcRelative.concat(".o"); // TODO: use the platform complaint object file extension

        fs::create_directories(obj.parent_path());

        Process::Result cmd = Process::run(context.compiler, headerDirs, cflagsFlattened, "-c", "-o", obj, src);

        if (cmd.exitcode != 0)
        {
            Terminate::failedProcess(cmd);
        }

        objectFiles.emplace_back(obj);
    }

    const std::string objectFilesFlattened =
        flattenStringEnumerable(objectFiles | Seq::map([](const fs::path& obj) { return obj.string(); }));

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
