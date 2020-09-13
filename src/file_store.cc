#include "file_store.h"

#include <fstream>

store::FileStore::FileStore(const std::string &globalPath_) : globalPath(globalPath_) {}

std::string store::FileStore::GetFullPath(const std::string &localPath)
{
    std::string res(globalPath + localPath);
    if (size_t pos = localPath.rfind('.'); pos == std::string::npos)  // dir
    {
        if (localPath.back() != '/')
        {
            res += '/';
        }
        res += "index.html";
    }
    return res;
}

std::pair<store::status, std::string> store::FileStore::Get(const std::string &fullPath) noexcept
{
    const std::lock_guard<std::mutex> guard(mutex);
    if (auto it = cache.find(fullPath); it != cache.end())
    {
        return it->second;
    }

    std::pair<store::status, std::string> res(store::status::ok, {});
    if (fullPath.rfind("/etc") != std::string::npos || fullPath.rfind("/..") != std::string::npos)
    {
        res.first = store::status::forbidden;
    }
    else
    {
        std::ifstream file(fullPath);
        if (!file)
        {
            res.first = fullPath.rfind("/index.html") != std::string::npos
                            ? store::status::forbidden
                            : store::status::notFound;
        }
        else
        {
            res.second = std::string((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());  // content
        }
    }

    cache.emplace(std::move(fullPath), res);
    return res;
}