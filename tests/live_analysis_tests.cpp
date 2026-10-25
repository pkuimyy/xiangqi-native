#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <xiangqi/live_analysis.hpp>
using namespace std::chrono_literals;
#define REQUIRE(x)                                                                                           \
    do {                                                                                                     \
        if (!(x))                                                                                            \
            throw std::runtime_error("Failed: " #x);                                                         \
    } while (false)
int main(int argc, char **argv) {
    try {
        REQUIRE(argc == 2);
        auto path = std::filesystem::u8path(argv[1]);
        std::string fen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";
        {
            xqn::UciEngine engine(path);
            std::atomic<bool> received = false;
            auto stream = std::async(std::launch::async, [&] {
                try {
                    engine.analyze_live(fen, [&](auto const &value) {
                        if (value.at("score") == 42 && value.at("bestmove") == "h2e2")
                            received = true;
                    });
                    return false;
                } catch (std::exception const &e) {
                    return std::string(e.what()) == "Analysis cancelled";
                }
            });
            for (int i = 0; i < 100 && !received; ++i)
                std::this_thread::sleep_for(10ms);
            engine.stop();
            REQUIRE(received);
            REQUIRE(stream.wait_for(2s) == std::future_status::ready);
            REQUIRE(stream.get());
            REQUIRE(engine.analyze(fen, 6).at("score") == 42);
        }
        std::mutex mutex;
        std::vector<xqn::LiveAnalysisEvent> events;
        auto handler = [&](auto const &event) {
            std::lock_guard lock(mutex);
            events.push_back(event);
        };
        auto waitFor = [&](std::uint64_t revision, xqn::LiveAnalysisEvent::Kind kind) {
            for (int i = 0; i < 200; ++i) {
                {
                    std::lock_guard lock(mutex);
                    for (auto const &event : events)
                        if (event.revision == revision && event.kind == kind)
                            return event;
                }
                std::this_thread::sleep_for(10ms);
            }
            throw std::runtime_error("Timed out waiting for live engine event");
        };
        {
            xqn::LiveAnalysis service(handler);
            auto first = service.request(path, fen);
            REQUIRE(waitFor(first, xqn::LiveAnalysisEvent::Kind::Update).value.at("score") == 42);
            auto stale = service.request(path, fen);
            auto black = fen;
            black.back() = 'b';
            auto current = service.request(path, black);
            auto result = waitFor(current, xqn::LiveAnalysisEvent::Kind::Update);
            REQUIRE(result.value.at("score") == -84);
            REQUIRE(result.value.at("bestmove") == "h7e7");
            {
                std::lock_guard lock(mutex);
                for (auto const &event : events)
                    REQUIRE(event.revision != stale);
            }
            service.stop();
            auto bad = service.request(path.parent_path() / "not-an-engine.exe", fen);
            waitFor(bad, xqn::LiveAnalysisEvent::Kind::Error);
            auto retry = service.request(path, fen);
            REQUIRE(waitFor(retry, xqn::LiveAnalysisEvent::Kind::Update).value.at("score") == 42);
            service.stop();
        }
#ifdef _WIN32
        _putenv_s("XQN_TEST_STALL_HANDSHAKE", "1");
        auto started = std::chrono::steady_clock::now();
        {
            xqn::LiveAnalysis stalled(handler);
            stalled.request(path, fen);
            std::this_thread::sleep_for(400ms);
        }
        REQUIRE(std::chrono::steady_clock::now() - started < 3s);
        _putenv_s("XQN_TEST_STALL_HANDSHAKE", "");
#endif
        std::cout << "PASS: live PV, stop/reuse, debounce, latest-position score, error recovery and "
                     "cancellable startup\n";
        return 0;
    } catch (std::exception const &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
