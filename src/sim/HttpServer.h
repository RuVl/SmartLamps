#pragma once
// Minimal HTTP server for the host simulator (native build only).
//
// Serves the preview page (page.h, generated from index.html by page.py),
// the JSON state API consumed by that page, and an SSE stream of frames.
// Single-threaded: serve() never blocks, broadcast() pushes one frame.
// Driven from sim/main.cpp: begin() once, then serve() + broadcast() per loop.
//
// Frame bytes come from a FrameGrabber installed by main.cpp — the server
// itself knows nothing about the LED driver. Until a grabber is installed,
// broadcast() is a no-op; the page and the state API work regardless.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class HttpServer
{
public:
    static constexpr uint16_t kPixels = MATRIX_WIDTH * MATRIX_HEIGHT;

    // Fills out with kPixels RGB triplets in matrix order: row by row from
    // the bottom-left, as Frame::at(x, y) sees it.
    using FrameGrabber = std::function<void(uint8_t* out)>;

    bool begin(uint16_t port);
    void close();

    void setFrameGrabber(FrameGrabber grabber) { grabber_ = std::move(grabber); }

    // Accepts and handles one waiting request, if any. Never blocks.
    void serve();

    // Pushes the current frame to all SSE clients. No-op without clients or
    // without a grabber.
    void broadcast();

private:
    static void reply(int fd, const char* status, const char* type, const std::string& body);
    void handleRequest(int fd, const std::string& request);
    std::string stateJson() const;

    int listener_ = -1;
    std::vector<int> streams_;
    FrameGrabber grabber_;
};
