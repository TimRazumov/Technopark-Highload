#pragma once

#include <string>
#include <tuple>
#include <map>
#include <mutex>

namespace store
{

enum class status
{
    ok,
    forbidden,
    notFound
};

class FileStore
{
public:
    explicit FileStore(const std::string &globalPath_);
    std::tuple<status, std::string, std::string> Get(const std::string &path) noexcept;
    
private:
    std::pair<std::string, std::string> GetFullPathAndFileType(const std::string &localPath);

    const std::string globalPath;
    std::map<std::string, std::tuple<status, std::string, std::string>> cache;
    std::mutex mutex;
};

}