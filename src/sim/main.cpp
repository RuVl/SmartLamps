#include <atomic>
#include <chrono>
#include <cstdio>
#include <csignal>
#include <thread>

#include "app/Lamp.h"
#include "hal/sim/SimFrame.h"

#include "HttpServer.h"

namespace
{
    constexpr int kFrameMs = 1000 / 60;
    constexpr uint16_t kPort = 8266;
    std::atomic_bool is_running{true};

    void sigint(int)
    {
        is_running = false;
    }

    uint32_t nowMs()
    {
        using namespace std::chrono;
        static const auto start = steady_clock::now();
        return uint32_t(duration_cast<milliseconds>(steady_clock::now() - start).count());
    }
}

int main()
{
    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);
    // A browser closing the SSE stream must not kill the simulator: with
    // SIGPIPE ignored, write() to a dead socket fails with EPIPE and
    // HttpServer::broadcast() drops the client on the next frame.
    signal(SIGPIPE, SIG_IGN);

    app::lamp().begin();

    HttpServer http;
    if (!http.begin(kPort))
    {
        fprintf(stderr, "sim: cannot listen on %u\n", kPort);
        return 1;
    }
    http.setFrameGrabber([](uint8_t* out) { hal::simSnapshot(out, app::lamp().indexMap()); });

    while (is_running)
    {
        const uint32_t now = nowMs();

        app::lamp().tick(now);
        app::lamp().render(now);

        http.serve();
        http.broadcast();

        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    http.close();
    return 0;
}
