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

    std::string GetFullPath(const std::string &localPath);
    std::pair<status, std::string> Get(const std::string &fullPath) noexcept;
    
private:
    const std::string globalPath;
    std::map<std::string, std::pair<status, std::string>> cache;
    std::mutex mutex;
};

}