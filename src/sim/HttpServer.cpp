#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <ArduinoJson.h>

#include "app/Lamp.h"
#include "core/Registry.h"

// Generated from src/sim/index.html by src/sim/page.py (pre-build script).
#include "page.h"

namespace
{
    // fetch() posts bare bodies ("Fire", "80", "speed=120"); strip any trailing
    // whitespace the transport may add so name lookups never miss on a '\n'.
    std::string trimmed(std::string s)
    {
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
        return s;
    }
}

bool HttpServer::begin(uint16_t port)
{
    listener_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_ < 0) return false;

    int yes = 1;
    setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(listener_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        close();
        return false;
    }
    listen(listener_, 64);

    printf("http://localhost:%u - %u effects\n", port, core::Registry::count());
    fflush(stdout);
    return true;
}

void HttpServer::close()
{
    for (int fd : streams_) ::close(fd);
    streams_.clear();
    if (listener_ >= 0)
    {
        ::close(listener_);
        listener_ = -1;
    }
}

void HttpServer::reply(int fd, const char* status, const char* type, const std::string& body)
{
    const std::string head = std::string("HTTP/1.1 ") + status +
        "\r\nContent-Type: " + type +
        "; charset=utf-8\r\nContent-Length: " + std::to_string(body.size()) +
        "\r\nConnection: close\r\n\r\n";
    (void)!write(fd, head.data(), head.size());
    (void)!write(fd, body.data(), body.size());
}

std::string HttpServer::stateJson() const
{
    JsonDocument doc;
    JsonArray effects = doc["effects"].to<JsonArray>();
    for (core::EffectInfo* e = core::Registry::head(); e != nullptr; e = e->next)
        effects.add(e->name);

    app::Lamp& lamp = app::lamp();
    doc["effect"] = lamp.effectName();
    doc["on"] = lamp.isOn();
    doc["brightness"] = lamp.brightness();
    doc["limit"] = lamp.currentLimit();

    JsonArray params = doc["params"].to<JsonArray>();
    lamp.forEachParam([&](core::Param& p)
    {
        JsonObject o = params.add<JsonObject>();
        o["key"] = p.key();
        o["id"] = p.key(); // the page addresses inputs by id
        o["label"] = p.label();
        o["min"] = p.min();
        o["max"] = p.max();
        o["value"] = p.get();
        static const char* const kinds[] = {"slider", "hue", "switch", "select"};
        o["kind"] = kinds[uint8_t(p.kind())];
        if (p.options() != nullptr) o["options"] = p.options();
    });

    std::string out;
    serializeJson(doc, out);
    return out;
}

void HttpServer::handleRequest(int fd, const std::string& request)
{
    const size_t eol = request.find("\r\n");
    const std::string line = request.substr(0, eol);
    const size_t firstSp = line.find(' ');
    const size_t lastSp = line.rfind(' ');
    const bool post = line.rfind("POST ", 0) == 0;
    const std::string path =
        (firstSp == std::string::npos || lastSp == std::string::npos || lastSp <= firstSp)
        ? std::string()
        : line.substr(firstSp + 1, lastSp - firstSp - 1);
    const size_t bodyAt = request.find("\r\n\r\n");
    const std::string body =
        bodyAt == std::string::npos ? std::string() : trimmed(request.substr(bodyAt + 4));

    app::Lamp& lamp = app::lamp();

    if (path == "/")
    {
        reply(fd, "200 OK", "text/html", kPage);
    }
    else if (path == "/events")
    {
        static const char head[] =
            "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\n\r\n";
        (void)!write(fd, head, strlen(head));
        streams_.push_back(fd);
        return; // stays open for the SSE stream
    }
    else if (path == "/api/state")
    {
        reply(fd, "200 OK", "application/json", stateJson());
    }
    else if (post && path == "/api/effect")
    {
        if (!body.empty()) lamp.selectEffect(body.c_str());
        noContent(fd);
    }
    else if (post && path == "/api/power")
    {
        lamp.setPower(atoi(body.c_str()) != 0);
        noContent(fd);
    }
    else if (post && path == "/api/brightness")
    {
        const int v = atoi(body.c_str());
        lamp.setBrightness(uint8_t(v < 0 ? 0 : (v > 100 ? 100 : v)));
        noContent(fd);
    }
    else if (post && path == "/api/limit")
    {
        // Milliamps; 0 switches the limiter off.
        const int v = atoi(body.c_str());
        lamp.setCurrentLimit(uint16_t(v < 0 ? 0 : (v > 65535 ? 65535 : v)));
        noContent(fd);
    }
    else if (post && path == "/api/param")
    {
        const size_t eq = body.find('=');
        if (eq != std::string::npos)
            lamp.setParam(body.substr(0, eq).c_str(), int16_t(atoi(body.c_str() + eq + 1)));
        noContent(fd);
    }
    else
    {
        reply(fd, "404 Not Found", "text/plain", "no");
    }
    ::close(fd);
}

void HttpServer::serve()
{
    if (listener_ < 0) return;
    // Everything queued, not one request per frame: a slider being dragged
    // posts faster than the lamp renders, and served one per loop the queue
    // grows and the picture trails the mouse by seconds.
    for (;;)
    {
        pollfd p{listener_, POLLIN, 0};
        if (poll(&p, 1, 0) <= 0) return;

        const int fd = accept(listener_, nullptr, nullptr);
        if (fd < 0) return;

        // The head and the body may arrive in separate segments; read until
        // Content-Length is satisfied (or the buffer is full).
        char buf[4096];
        size_t got = 0;
        size_t need = sizeof(buf) - 1;
        while (got < need)
        {
            const ssize_t n = read(fd, buf + got, sizeof(buf) - 1 - got);
            if (n <= 0) break;
            got += size_t(n);
            buf[got] = 0;
            const char* headEnd = strstr(buf, "\r\n\r\n");
            if (headEnd == nullptr) continue;
            const char* cl = strcasestr(buf, "Content-Length:");
            const size_t bodyLen = (cl != nullptr && cl < headEnd) ? size_t(atoi(cl + 15)) : 0;
            need = std::min(sizeof(buf) - 1, size_t(headEnd + 4 - buf) + bodyLen);
        }
        if (got == 0)
        {
            ::close(fd);
            continue;
        }
        buf[got] = 0;
        handleRequest(fd, std::string(buf, got));
    }
}

void HttpServer::broadcast()
{
    if (streams_.empty() || !grabber_) return;

    uint8_t rgb[kPixels * 3];
    grabber_(rgb);

    static const char* hex = "0123456789abcdef";
    std::string msg("data: ");
    msg.reserve(8 + kPixels * 6);
    for (uint8_t v : rgb)
    {
        msg += hex[v >> 4];
        msg += hex[v & 15];
    }
    msg += "\n\n";
    // The new effect exists only after the fade-out; the page must not read
    // its parameters before that. Same fix as the lamp's panel (WebUi.cpp).
    if (app::lamp().consumeActivated()) msg += "event: state\ndata: 1\n\n";

    for (size_t i = 0; i < streams_.size();)
    {
        if (write(streams_[i], msg.data(), msg.size()) < 0)
        {
            ::close(streams_[i]);
            streams_.erase(streams_.begin() + long(i));
        }
        else
        {
            ++i;
        }
    }
}
