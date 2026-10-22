#include <future>
#include <iostream>
#include <thread>
#include <xiangqi/uci.hpp>
#define REQUIRE(c)                                                                                           \
    do {                                                                                                     \
        if (!(c))                                                                                            \
            throw std::runtime_error("Failed: " #c);                                                         \
    } while (false)
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            return 2;
        auto path = std::filesystem::u8path(argv[1]);
        xqn::UciEngine engine(path);
        const std::string fen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";
        auto r = engine.analyze(fen, 8, 20, 2);
        REQUIRE(r.at("bestmove") == "h2e2");
        REQUIRE(r.at("ponder") == "h7e7");
        REQUIRE(r.at("score") == 42);
        REQUIRE(r.at("pvs").size() == 2);
        REQUIRE(r.at("pvs").at(1).at("scoreType") == "mate");
        auto black = xqn::red_view(r, "4k4/9/9/9/9/9/9/9/9/4K4 b");
        REQUIRE(black.at("score") == -42);
        REQUIRE(black.at("pvs").at(1).at("score") == 3);
        REQUIRE(xqn::red_view({{"score", 0}, {"scoreType", "mate"}}, fen).at("score") == -1);
        auto cancelled = std::async(std::launch::async, [&] {
            try {
                engine.analyze(fen, 30);
                return false;
            } catch (const std::exception &e) {
                return std::string(e.what()) == "Analysis cancelled";
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        engine.stop();
        REQUIRE(cancelled.get());
        REQUIRE(engine.analyze(fen, 5).at("score") == 42);
        xqn::UciEngine quiet(path);
        bool timedOut = false;
        try {
            quiet.analyze(fen, 29, 20, 1, std::chrono::milliseconds(150));
        } catch (const std::exception &e) {
            timedOut = std::string(e.what()) == "Engine response timeout";
        }
        REQUIRE(timedOut);
        std::cout << "PASS: UCI handshake, exact scores, MultiPV, ponder, red POV, cancellation, restart and "
                     "timeout\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
