#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include "input_action.h"
namespace capsime {
enum class Request { reject, health, preflight, toggle, key };
inline Request classify_request(const std::string& request, KeyAction* action = nullptr) {
    const auto end=request.find("\r\n\r\n");
    if(end==std::string::npos || end>8192) return Request::reject;
    const auto first=request.find("\r\n");
    const auto line=request.substr(0,first);
    std::map<std::string,std::string> headers;
    size_t start=first+2;
    while(start<end) {
        const auto next=request.find("\r\n",start);
        const auto colon=request.find(':',start);
        if(colon==std::string::npos || colon>=next) return Request::reject;
        auto name=request.substr(start,colon-start);
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        auto value=request.substr(colon+1,next-colon-1);
        const auto a=value.find_first_not_of(" \t");
        value=a==std::string::npos ? "" : value.substr(a,value.find_last_not_of(" \t")-a+1);
        if(!headers.emplace(name,value).second) return Request::reject;
        start=next+2;
    }
    if(headers["host"]!="127.0.0.1:27391" || headers.count("transfer-encoding") ||
       (headers.count("content-length") && headers["content-length"]!="0")) return Request::reject;
    if(line=="GET /health HTTP/1.1") return Request::health;
    if(headers["origin"]!="https://steamloopback.host") return Request::reject;
    const auto path_start = line.find(' ');
    const auto path_end = line.rfind(' ');
    if (path_start == std::string::npos || path_end <= path_start || line.substr(path_end) != " HTTP/1.1") {
        return Request::reject;
    }
    const auto method = line.substr(0, path_start);
    const auto path = line.substr(path_start + 1, path_end - path_start - 1);
    const auto key = path.compare(0, 5, "/key/") == 0 ? parse_key(std::string_view(path).substr(5)) : KeyAction{};
    if (path != "/toggle" && key.key == Key::none) return Request::reject;
    if (method == "OPTIONS" && headers["access-control-request-method"] == "POST") return Request::preflight;
    if (method == "POST" && headers["x-steamvr-caps-ime"] == "1") {
        if (path == "/toggle") return Request::toggle;
        if (action) *action = key;
        return Request::key;
    }
    return Request::reject;
}
}
