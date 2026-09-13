// Host simulator: runs the real core/ and effects/ on the PC and shows the
// matrix in a browser or in the terminal. Nothing here ships to a board.
//
//   pio run -e sim
//   .pio/build/sim/program --serve        # http://localhost:8266
//   .pio/build/sim/program --tty [name]   # ANSI rendering in the terminal
//
// The frame goes through the same gamma as on the lamp; the current limiter
// is not modelled — the simulator has no power supply to run out of.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <FastLED.h>

#include "core/Frame.h"
#include "core/Matrix.h"
#include "core/PostFX.h"
#include "core/Registry.h"

namespace {

constexpr uint16_t kCount = MATRIX_WIDTH * MATRIX_HEIGHT;
constexpr int kFrameMs = 1000 / 60;

// ---------------------------------------------------------------- the lamp --

struct Sim {
    CRGB pixels[kCount] = {};
    uint16_t map[kCount] = {};
    core::Geometry geo{MATRIX_WIDTH, MATRIX_HEIGHT, core::MatrixType::Serpentine,
                       core::Corner::BottomLeft, core::Direction::Right};
    alignas(8) uint8_t arena[EFFECT_ARENA_SIZE] = {};
    core::Effect* effect = nullptr;
    core::EffectInfo* info = nullptr;
    uint8_t brightnessPercent = 100;

    Sim() { core::buildIndexMap(map, geo); }

    core::Frame frame() { return core::Frame(pixels, map, geo); }

    void select(core::EffectInfo* next) {
        if (next == nullptr) return;
        if (effect != nullptr) effect->~Effect();
        info = next;
        effect = next->construct(arena);
        core::Frame f = frame();
        f.clear();
        effect->begin(f);
    }

    void render(uint16_t dtMs) {
        core::Frame f = frame();
        if (effect != nullptr) effect->render(f, dtMs);
    }

    // What the strip would receive: gamma-corrected brightness applied.
    void output(uint8_t* out) const {
        const uint8_t b = core::gammaCorrect(brightnessPercent);
        for (uint16_t i = 0; i < kCount; ++i) {
            out[i * 3 + 0] = scale8(pixels[i].r, b);
            out[i * 3 + 1] = scale8(pixels[i].g, b);
            out[i * 3 + 2] = scale8(pixels[i].b, b);
        }
    }
};

// ------------------------------------------------------------- terminal --

void drawTty(const Sim& sim) {
    uint8_t rgb[kCount * 3];
    sim.output(rgb);
    std::string s = "\x1b[H";  // home
    // Two matrix rows per text row: upper half block, fg = top, bg = bottom.
    for (int y = MATRIX_HEIGHT - 1; y >= 0; y -= 2) {
        for (int x = 0; x < MATRIX_WIDTH; ++x) {
            const uint16_t top = sim.map[y * MATRIX_WIDTH + x];
            const uint16_t bot = sim.map[(y - 1) * MATRIX_WIDTH + x];
            char buf[64];
            snprintf(buf, sizeof(buf), "\x1b[38;2;%u;%u;%um\x1b[48;2;%u;%u;%um\xe2\x96\x80\xe2\x96\x80",
                     rgb[top * 3], rgb[top * 3 + 1], rgb[top * 3 + 2],
                     rgb[bot * 3], rgb[bot * 3 + 1], rgb[bot * 3 + 2]);
            s += buf;
        }
        s += "\x1b[0m\n";
    }
    fputs(s.c_str(), stdout);
    fflush(stdout);
}

// ------------------------------------------------------------------ http --

const char* kPage = R"HTML(<!doctype html><meta charset="utf-8"><title>SmartLamp sim</title>
<style>
body{margin:0;background:#141318;color:#ddd;font:14px system-ui;display:flex;gap:28px;padding:24px}
canvas{image-rendering:pixelated;width:480px;height:480px;background:#000;border:1px solid #333}
label{display:block;margin:10px 0 2px;color:#aaa}input[type=range]{width:260px}
select{font:inherit;padding:4px}#fps{color:#777;margin-top:16px}
</style>
<canvas id=c width=16 height=16></canvas>
<div>
<label>Эффект</label><select id=fx></select>
<label>Яркость <span id=bv></span>%</label><input id=b type=range min=0 max=100>
<div id=params></div><div id=fps></div>
</div>
<script>
const c=document.getElementById('c').getContext('2d'),img=c.createImageData(16,16);
const q=(u,b)=>fetch(u,{method:'POST',body:b});
async function load(){const s=await (await fetch('/api/state')).json();
 const fx=document.getElementById('fx');fx.innerHTML='';
 for(const n of s.effects){const o=document.createElement('option');o.textContent=n;o.selected=n===s.effect;fx.append(o)}
 fx.onchange=()=>q('/api/effect',fx.value).then(load);
 const b=document.getElementById('b');b.value=s.brightness;bv.textContent=s.brightness;
 b.oninput=()=>{bv.textContent=b.value;q('/api/brightness',b.value)};
 const p=document.getElementById('params');p.innerHTML='';
 for(const x of s.params){const l=document.createElement('label');const v=document.createElement('span');
  l.textContent=x.label+' ';l.append(v);v.textContent=x.value;
  const r=document.createElement('input');r.type='range';r.min=x.min;r.max=x.max;r.value=x.value;
  r.oninput=()=>{v.textContent=r.value;q('/api/param',x.key+'='+r.value)};p.append(l,r)}}
let n=0,t=performance.now();
new EventSource('/events').onmessage=e=>{const h=e.data;
 for(let i=0;i<256;i++){const y=15-(i>>4),x=i&15,o=(y*16+x)*4;
  img.data[o]=parseInt(h.substr(i*6,2),16);img.data[o+1]=parseInt(h.substr(i*6+2,2),16);
  img.data[o+2]=parseInt(h.substr(i*6+4,2),16);img.data[o+3]=255}
 c.putImageData(img,0,0);if(++n%60==0){const now=performance.now();fps.textContent=Math.round(60000/(now-t))+' к/с';t=now}};
load();
</script>)HTML";

struct Http {
    int listener = -1;
    std::vector<int> streams;  // SSE clients

    bool begin(uint16_t port) {
        listener = socket(AF_INET, SOCK_STREAM, 0);
        int yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons(port);
        if (bind(listener, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) return false;
        listen(listener, 8);
        return true;
    }

    static void reply(int fd, const char* status, const char* type, const std::string& body) {
        std::string h = std::string("HTTP/1.1 ") + status + "\r\nContent-Type: " + type +
                        "; charset=utf-8\r\nContent-Length: " + std::to_string(body.size()) +
                        "\r\nConnection: close\r\n\r\n" + body;
        (void)!write(fd, h.data(), h.size());
    }

    // Handles one request if one is waiting. Never blocks the render loop.
    void serve(Sim& sim) {
        pollfd p{listener, POLLIN, 0};
        if (poll(&p, 1, 0) <= 0) return;
        const int fd = accept(listener, nullptr, nullptr);
        if (fd < 0) return;

        char buf[4096];
        const ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) { close(fd); return; }
        buf[n] = 0;
        std::string req(buf);
        const std::string line = req.substr(0, req.find("\r\n"));
        const bool post = line.rfind("POST ", 0) == 0;
        const std::string path = line.substr(line.find(' ') + 1, line.rfind(' ') - line.find(' ') - 1);
        const size_t bodyAt = req.find("\r\n\r\n");
        const std::string body = bodyAt == std::string::npos ? "" : req.substr(bodyAt + 4);

        if (path == "/") {
            reply(fd, "200 OK", "text/html", kPage);
        } else if (path == "/events") {
            const char* h = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\n\r\n";
            (void)!write(fd, h, strlen(h));
            streams.push_back(fd);
            return;  // stays open
        } else if (path == "/api/state") {
            std::string j = "{\"effects\":[";
            for (core::EffectInfo* e = core::Registry::head(); e; e = e->next)
                j += std::string(e == core::Registry::head() ? "" : ",") + "\"" + e->name + "\"";
            j += "],\"effect\":\"" + std::string(sim.info ? sim.info->name : "") + "\"";
            j += ",\"brightness\":" + std::to_string(sim.brightnessPercent) + ",\"params\":[";
            bool first = true;
            for (core::Param* q = sim.effect ? sim.effect->params() : nullptr; q; q = q->next()) {
                j += std::string(first ? "" : ",") + "{\"key\":\"" + q->key() + "\",\"label\":\"" + q->label() +
                     "\",\"min\":" + std::to_string(q->min()) + ",\"max\":" + std::to_string(q->max()) +
                     ",\"value\":" + std::to_string(q->get()) + "}";
                first = false;
            }
            reply(fd, "200 OK", "application/json", j + "]}");
        } else if (post && path == "/api/effect") {
            sim.select(core::Registry::find(body.c_str()));
            reply(fd, "204 No Content", "text/plain", "");
        } else if (post && path == "/api/brightness") {
            sim.brightnessPercent = uint8_t(std::min(100, std::max(0, atoi(body.c_str()))));
            reply(fd, "204 No Content", "text/plain", "");
        } else if (post && path == "/api/param") {
            const size_t eq = body.find('=');
            if (eq != std::string::npos && sim.effect) {
                const std::string key = body.substr(0, eq);
                for (core::Param* q = sim.effect->params(); q; q = q->next())
                    if (key == q->key()) q->set(int16_t(atoi(body.c_str() + eq + 1)));
            }
            reply(fd, "204 No Content", "text/plain", "");
        } else {
            reply(fd, "404 Not Found", "text/plain", "no");
        }
        close(fd);
    }

    void broadcast(const Sim& sim) {
        if (streams.empty()) return;
        uint8_t rgb[kCount * 3];
        sim.output(rgb);
        static const char* hex = "0123456789abcdef";
        std::string msg = "data: ";
        msg.reserve(8 + kCount * 6 + 2);
        for (uint8_t v : rgb) { msg += hex[v >> 4]; msg += hex[v & 15]; }
        msg += "\n\n";
        for (size_t i = 0; i < streams.size();) {
            if (write(streams[i], msg.data(), msg.size()) < 0) {
                close(streams[i]);
                streams.erase(streams.begin() + long(i));
            } else {
                ++i;
            }
        }
    }
};

}  // namespace

int main(int argc, char** argv) {
    const bool tty = argc > 1 && strcmp(argv[1], "--tty") == 0;
    const bool serve = argc > 1 && strcmp(argv[1], "--serve") == 0;
    if (!tty && !serve) {
        fprintf(stderr, "usage: %s --serve [port] | --tty [effect]\n", argv[0]);
        return 2;
    }

    Sim sim;
    core::EffectInfo* first = core::Registry::head();
    if (first == nullptr) { fprintf(stderr, "no effects registered\n"); return 1; }
    sim.select(tty && argc > 2 ? core::Registry::find(argv[2]) : first);
    if (sim.effect == nullptr) { fprintf(stderr, "unknown effect\n"); return 1; }

    Http http;
    if (serve) {
        const uint16_t port = argc > 2 ? uint16_t(atoi(argv[2])) : 8266;
        if (!http.begin(port)) { perror("bind"); return 1; }
        printf("http://localhost:%u  — %u effects, showing \"%s\"\n", port, core::Registry::count(), sim.info->name);
    } else {
        fputs("\x1b[2J\x1b[?25l", stdout);  // clear, hide cursor
    }

    auto last = std::chrono::steady_clock::now();
    for (;;) {
        const auto now = std::chrono::steady_clock::now();
        const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        last = now;
        sim.render(uint16_t(dt));
        if (serve) { http.serve(sim); http.broadcast(sim); }
        else drawTty(sim);
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }
}
