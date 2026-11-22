#include "module.hpp"

#include <string_view>
#include <filesystem>
#include <fstream>
#include <vector>

#include "fmt-ext.hpp"
#include "process.hpp"
#include "seq/seq.hpp"
#include "terminate.hpp"
#include "stringutils.hpp"

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

    if (sourceNodes.empty())
    {
        Terminate::withMessage("Empty sources array encountered", TerminateConditionWhen::AT_CONFIGURE);
    }

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

static auto populateObjectFiles(const std::vector<fs::path>& sources, const BuildContext& context) -> std::vector<fs::path>
{
    std::vector<fs::path> objs;
    objs.reserve(sources.size());

    for (const auto& src : sources)
    {
        fs::path srcRelative = fs::relative(src, context.sourceDir);
        objs.emplace_back(context.buildDir / "obj" / srcRelative.concat(Platform::objectFileExtension()));

        fs::create_directories(objs.back().parent_path());
    }

    return objs;
}

static auto parseModuleType(std::string_view type) -> ModuleType
{
    if (type == "executable")
    {
        return ModuleType::EXECUTABLE;
    }
    else if (type == "dynamic-lib")
    {
        return ModuleType::DYNAMIC_LIB;
    }
    else if (type == "static-lib")
    {
        return ModuleType::STATIC_LIB;
    }

    Terminate::invalidModuleType(type);
}

auto BuildModule::linkStaticLibrary() const -> void
{
    const std::string objectFilesFlattened =
        flattenStringEnumerable(objectFiles | Seq::map([](const fs::path& obj) { return obj.string(); }));

    Process::Input link = Process::create(context.archiver, "-r", "-c", "-s", context.buildDir / output, objectFilesFlattened);
    fmt::println("{:s}", link);

    Process::Result cmd = Process::run(link);

    if (cmd.exitcode != 0)
    {
        Terminate::failedProcess(cmd);
    }
}

auto BuildModule::linkStaticExecutable() const -> void
{
    const std::string objectFilesFlattened =
        flattenStringEnumerable(objectFiles | Seq::map([](const fs::path& obj) { return fmt::format("{:s}", obj); }));

    Process::Input link;

    if (String::contains(context.compiler, "cl.exe"))
    {
        link = Process::create(
            context.linker,
            fmt::format("/OUT:{:s}", context.buildDir / output),
            objectFilesFlattened
        );
    }
    else if (String::contains(context.compiler, "mingw"))
    {
        Process::Result cmdCrt2 = Process::run(Process::create(context.compiler, "--print-file-name", "crt2.o"));
        Process::Result cmdCrtb = Process::run(Process::create(context.compiler, "--print-file-name", "crtbegin.o"));
        Process::Result cmdCrte = Process::run(Process::create(context.compiler, "--print-file-name", "crtend.o"));

        fs::path baseLibDir = fs::path{cmdCrt2.output}.parent_path();
        fs::path gccLibDir  = fs::path{cmdCrtb.output}.parent_path();

        link = Process::create(
            context.linker,
            "-static",
            "-o", context.buildDir / output,
            "-L", baseLibDir,
            "-L", gccLibDir,
            objectFilesFlattened,
            cmdCrt2.output,
            cmdCrtb.output,
            cmdCrte.output,
            "-lstdc++",
            "--start-group", "-lgcc", "-lgcc_eh", "--end-group",
            "-lmingw32",
            "-lpthread",
            "-lmingwex",
            "-lmsvcrt",
            "-lkernel32",
            "-ladvapi32",
            "-lshell32",
            "-luser32"
        );
    }
    else
    {
        Process::Result cmdCrt1 = Process::run(Process::create(context.compiler, "--print-file-name", "crt1.o"));
        Process::Result cmdCrti = Process::run(Process::create(context.compiler, "--print-file-name", "crti.o"));
        Process::Result cmdCrtn = Process::run(Process::create(context.compiler, "--print-file-name", "crtn.o"));
        Process::Result cmdCrtb = Process::run(Process::create(context.compiler, "--print-file-name", "crtbeginT.o"));
        Process::Result cmdCrte = Process::run(Process::create(context.compiler, "--print-file-name", "crtend.o"));

        fs::path baseLibDir = fs::path{cmdCrt1.output}.parent_path();
        fs::path gccLibDir  = fs::path{cmdCrtb.output}.parent_path();

        link = Process::create(
            context.linker,
            "-static",
            "-o", context.buildDir / output,
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
    }

    // fmt::println("{:s}", link);

    Process::Result cmd = Process::run(link);

    if (cmd.exitcode != 0)
    {
        Terminate::failedProcess(cmd);
    }
}

BuildModule::BuildModule(const std::string& moduleName, const toml::table& module, const BuildContext& context)
    : context(context)
    , sources(parseSourceFiles(getTomlProperty<toml::array>(module, "sources"), context))
    , headers(parseHeaderDirs(getTomlProperty<toml::array, TomlLookupManner::MAYBE_SPECIFIED>(module, "headers").value_or(toml::array{}), context))
    , objectFiles(populateObjectFiles(sources, context))
    , cflags(getTomlProperty<toml::array, TomlLookupManner::MAYBE_SPECIFIED>(module, "cflags").value_or(toml::array{}))
    , output(getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(module, "output").value_or(moduleName))
    , type(parseModuleType(getTomlProperty<std::string>(module, "type")))
{
    fs::create_directories(context.buildDir);
}

auto BuildModule::build() const -> void
{
    std::size_t finished    = 0;
    const std::size_t total = sources.size();

    const std::string cflagsFlattened = TomlExt::flattenStringArray(cflags);

    const std::string headerDirs =
        flattenStringEnumerable(headers | Seq::map([](const fs::path& hdr) { return "\"-I" + hdr.string() + "\""; }));

    const std::string compileFlag =
        context.compiler == "cl.exe"
        ? "/c"
        : "-c";

    const std::string outputFlag =
        context.compiler == "cl.exe"
        ? "/Fo:"
        : "-o";

    for (std::size_t i = 0; i < total; ++i)
    {
        const auto& src = sources[i];
        const auto& obj = objectFiles[i];

        Process::Input compile = Process::create(
            context.compiler,
            headerDirs,
            cflagsFlattened,
            compileFlag,
            outputFlag,
            fmt::format("{:s}", obj),
            fmt::format("{:s}", src)
        );

        fmt::println("[{:d}/{:d}] {:s}", finished, total, compile);

        Process::Result cmd = Process::run(compile);

        if (cmd.exitcode != 0)
        {
            Terminate::failedProcess(cmd);
        }

        finished += 1;
    }

    // TODO: check whether we actually need linking with stdc++
    // TODO: collect object files for linking step and flatten them to single param
    // ---
}

auto BuildModule::link() const -> void
{
    switch (type)
    {
        case ModuleType::STATIC_LIB:
            linkStaticLibrary();
            break;
        case ModuleType::EXECUTABLE:
            linkStaticExecutable();
            break;
        default:
            break;
    }
}

auto BuildModule::generateCompileCommandsJson(std::size_t indent) const -> void
{
    fs::path compileCommands = context.buildDir / "compile_commands.json";

    {
        std::ofstream fileHandle{compileCommands};
        std::string indentTxt(indent, ' ');

        if (not fileHandle.is_open())
        {
            Terminate::withMessage("Failed to open compile_commands.json for writing", TerminateConditionWhen::AT_BUILD);
        }

        fmt::println(fileHandle, "[");

        sources
        | Seq::iteri([&fileHandle, &indentTxt, this](const fs::path& src, std::size_t i) {
                const fs::path srcRelative = fs::relative(src, context.buildDir);

                IEnumerable<std::string> headerFlags =
                    headers
                    | Seq::map([this](const fs::path& hdr) {
                            return "-I" + fs::relative(hdr, context.buildDir).string();
                        });

                IEnumerable<std::string> cflagsFlags =
                    cflags
                    | Seq::map([](const toml::node& node) {
                            return *node.value_exact<std::string>();
                        });

                std::vector<std::string> arguments;
                arguments.push_back(context.compiler);

                for (const auto& hdr : headerFlags)
                {
                    arguments.push_back(hdr);
                }

                for (const auto& flag : cflagsFlags)
                {
                    arguments.push_back(flag);
                }

                arguments.push_back("/c");
                arguments.push_back(srcRelative.string());

                if (i != 0)
                {
                    fmt::println(fileHandle, ",");
                }

                // LINE #1
                // ---
                fmt::print(fileHandle, "{:s}", indentTxt);
                fmt::print(fileHandle, "{{ ");
                fmt::println(fileHandle, "\"directory\": {:s},", context.buildDir);

                // LINE #2
                // ---
                fmt::print(fileHandle, "{:s}", indentTxt);
                fmt::print(fileHandle, "{:s}", indentTxt);
                fmt::println(fileHandle, "\"arguments\": {},", arguments);

                // LINE #3
                // ---
                fmt::print(fileHandle, "{:s}", indentTxt);
                fmt::print(fileHandle, "{:s}", indentTxt);
                fmt::print(fileHandle, "\"file\": {:s}", srcRelative);
                fmt::print(fileHandle, " }}");
            });

        fmt::println(fileHandle, "\n" "]");
    }
}
