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
    capsime::KeyAction action;
    assert(capsime::classify_request(request("POST /key/space HTTP/1.1", headers), &action) == Request::key);
    assert(action.key == capsime::Key::space && !action.shift);
    assert(capsime::classify_request(request("POST /key/tab-shift HTTP/1.1", headers), &action) == Request::key);
    assert(action.key == capsime::Key::tab && action.shift);
    assert(capsime::classify_request(request("POST /key/ascii-61 HTTP/1.1", headers), &action) == Request::key);
    assert(action.key == capsime::Key::ascii && action.character == 'a');
    for (const auto* path : {"ascii-0d", "ascii-7f", "win", "ctrl-tab", "tab?foo", "ascii-zz"}) {
        assert(capsime::classify_request(request(std::string("POST /key/") + path + " HTTP/1.1", headers)) == Request::reject);
    }
    assert(capsime::classify_request(request("POST /key/enter HTTP/1.1", "Origin: https://example.com\r\nX-SteamVR-Caps-IME: 1\r\n")) == Request::reject);
    assert(capsime::classify_request("malformed")==Request::reject);
    puts("PASS: only Steam loopback POST can toggle; health/preflight and foreign requests cannot");
}
