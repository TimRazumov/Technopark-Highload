#include "utils.h"

#include <ctime>
#include <sstream>

std::string utils::MakePathByUrl(const std::string& url)
{
    std::string decodedUrl = utils::DecodeUrl(url);
    size_t pos = decodedUrl.find('?');
    if (pos != std::string::npos)
    {
        return decodedUrl.substr(0, pos);
    }
    return  decodedUrl;
}

std::string utils::DecodeUrl(const std::string& url)
{
    std::string res;
    for (size_t i = 0; i < url.size(); ++i)
    {
        if (url[i] == '%')
        {
            if (i + 3 <= url.size())
            {
                int value = 0;
                std::istringstream is(url.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    res += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    return {};
                }
            }
            else
            {
                return {};
            }
        }
        else if (url[i] == '+')
        {
            res += ' ';
        }
        else
        {
            res += url[i];
        }
    }
    return res;
}

std::string utils::GetTimeNow()
{
    time_t time = std::time(nullptr);
    std::string timeFormat = std::ctime(&time);
    return timeFormat.substr(0, timeFormat.size() - 1); // without /n
}

std::string utils::GetContentType(const std::string &fullPath)
{
    std::string res("application/unknown");

    size_t pos = fullPath.rfind('.');
    if (pos == std::string::npos || pos == fullPath.size() - 1)
    {
        return res;
    }
    std::string fileType = fullPath.substr(pos + 1);

    if (fileType == "html" || fileType == "css")
    {
        res = "text/" + fileType;
    }
    else if (fileType == "gif" || fileType == "jpeg" || fileType == "png")
    {
        res = "image/" + fileType;
    }
    else if (fileType == "jpg")
    {
        res = "image/jpeg";
    }
    else if (fileType == "js")
    {
        res  = "application/javascript";
    }
    else if (fileType == "swf")
    {
        res  = "application/x-shockwave-flash";
    }
    return res;
}