#include "../src/http_policy.h"
#include <cassert>
#include <cstdio>
using capsime::Request;
std::string request(const std::string& first,const std::string& extra="") {
    return first+"\r\nHost: 127.0.0.1:27391\r\n"+extra+"\r\n";
}
int main() {
    assert(capsime::classify_request(request("GET /health HTTP/1.1"))==Request::health);
    const std::string headers="Origin: https://steamloopback.host\r\nX-SteamVR-Caps-IME: 1\r\n";
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1",headers))==Request::toggle);
    assert(capsime::classify_request(request("GET /toggle HTTP/1.1",headers))==Request::reject);
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1"))==Request::reject);
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1","Origin: https://example.com\r\nX-SteamVR-Caps-IME: 1\r\n"))==Request::reject);
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1",headers+"Origin: https://example.com\r\n"))==Request::reject);
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1",headers+"Content-Length: 1\r\n"))==Request::reject);
    assert(capsime::classify_request(request("POST /toggle HTTP/1.1",headers+"Transfer-Encoding: chunked\r\n"))==Request::reject);
    assert(capsime::classify_request(request("OPTIONS /toggle HTTP/1.1","Origin: https://steamloopback.host\r\nAccess-Control-Request-Method: POST\r\n"))==Request::preflight);
    assert(capsime::classify_request("malformed")==Request::reject);
    puts("PASS: only Steam loopback POST can toggle; health/preflight and foreign requests cannot");
}
