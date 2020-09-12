#include "file_store.h"

#include <fstream>

store::FileStore::FileStore(const std::string &globalPath_) : globalPath(globalPath_)
{
}

std::tuple<store::status, std::string, std::string> store::FileStore::Get(const std::string &path) noexcept
{
    const std::lock_guard<std::mutex> guard(mutex); 
    auto [fullPath, fileType] = GetFullPathAndFileType(path);
    if (cache.find(fullPath) == cache.end())
    {
        std::ifstream file(fullPath);
        if (!file)
        {
            cache.emplace(fullPath, std::make_tuple<store::status, std::string, std::string>(store::status::notFound, {}, {}));   
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        cache.emplace(fullPath, std::make_tuple<store::status, std::string, std::string>(store::status::ok, std::move(content), std::move(fileType)));
    }
    return cache[fullPath];
}

std::pair<std::string, std::string> store::FileStore::GetFullPathAndFileType(const std::string &localPath)
{
    std::pair<std::string, std::string> res(globalPath + localPath, "html");
    if (size_t pos = localPath.rfind('.'); pos != std::string::npos) // file
    {
        res.second = localPath.substr(pos);
    }
    else // dir
    {
        if (localPath.back() != '/')
        {
            res.first += '/';
        }
        res.first += "index.html";
    }
    return res;
}