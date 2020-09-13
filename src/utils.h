#pragma once

#include <string>

namespace utils
{
std::string MakePathByUrl(const std::string &url);
std::string DecodeUrl(const std::string &url);

std::string GetTimeNow();

std::string GetContentType(const std::string &fullPath);

}  // namespace utils
