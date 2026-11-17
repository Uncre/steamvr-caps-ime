#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
namespace capsime {
enum class Request { reject, health, preflight, toggle };
inline Request classify_request(const std::string& request) {
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
    if(line=="OPTIONS /toggle HTTP/1.1" && headers["access-control-request-method"]=="POST") return Request::preflight;
    if(line=="POST /toggle HTTP/1.1" && headers["x-steamvr-caps-ime"]=="1") return Request::toggle;
    return Request::reject;
}
}
