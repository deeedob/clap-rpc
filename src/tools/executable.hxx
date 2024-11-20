#include <clap-rpc/tools/executable.hpp>

#include <filesystem>

CLAP_RPC_BEGIN_NAMESPACE

class ExecutablePrivate
{
public:
    ExecutablePrivate() = default;
    explicit ExecutablePrivate(std::string_view path, std::initializer_list<std::string_view> args)
        : mPath(path), mArgs(args.begin(), args.end())
    {
    }

    bool isValid();

    // Static Interface Begin
    ~ExecutablePrivate();
    bool exec();
    std::optional<int> kill() const;
    [[nodiscard]] std::optional<int> updateStatus();
    void reset();
    [[nodiscard]] std::optional<Executable::PidType> pid() const noexcept;
    // Static Interface End


    std::filesystem::path mPath;
    std::vector<std::string> mArgs;
#if defined _WIN32 || defined _WIN64
    DWORD mPid;
#elif defined __linux__ || defined __APPLE__
    pid_t mPid = -1;
#endif
    Executable::Error mError = Executable::Error::None;
    Executable::Status mStatus = Executable::Status::Init;
};

inline bool ExecutablePrivate::isValid()
{
    if (mPath.empty() || !std::filesystem::exists(mPath)) {
        mError = Executable::Error::NotFound;
        return false;
    }
    // Check if the file has executable permissions for the current user
    auto filePermissions = std::filesystem::status(mPath).permissions();
    if ((filePermissions & std::filesystem::perms::owner_exec) == std::filesystem::perms::none
        && (filePermissions & std::filesystem::perms::group_exec) == std::filesystem::perms::none
        && (filePermissions & std::filesystem::perms::others_exec)
            == std::filesystem::perms::none) {
        mError = Executable::Error::Permission;
        return false;
    }
    return true;
}

CLAP_RPC_END_NAMESPACE
