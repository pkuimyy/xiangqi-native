#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <xiangqi/kernel.hpp>
using xqn::Json;
void require_equal(const Json &a, const Json &b, const std::string &context) {
    if (a != b) {
        std::cerr << context << "\nactual: " << a.dump() << "\nexpected: " << b.dump() << "\n";
        throw std::runtime_error(context);
    }
}
int main(int argc, char **argv) {
    try {
        std::ifstream input(argv[1]);
        Json fixture;
        input >> fixture;
        xqn::Kernel kernel;
        size_t count = 0;
        for (auto &p : fixture["positions"]) {
            auto actual = kernel.call("position", {{"fen", p["fen"]}});
            auto moves = actual["moves"], expected = p["moves"];
            auto sort = [](Json &v) {
                std::sort(v.begin(), v.end(), [](auto &a, auto &b) {
                    return a["lan"].template get<std::string>() < b["lan"].template get<std::string>();
                });
            };
            sort(moves);
            sort(expected);
            require_equal(moves, expected, "legal moves / notation position " + std::to_string(count));
            require_equal(actual["check"], p["check"], "check");
            auto after = kernel.call("move", {{"fen", p["fen"]}, {"notation", p["after"]["move"]["lan"]}});
            require_equal(after, p["after"], "after move");
            auto chinese = kernel.call("move", {{"fen", p["fen"]}, {"notation", p["after"]["move"]["zh"]}});
            require_equal(chinese, after, "Chinese notation replay");
            ++count;
        }
        for (auto &p : fixture["special"]) {
            auto actual = kernel.call("position", {{"fen", p["fen"]}}), expected = p["position"];
            auto sort = [](Json &v) {
                std::sort(v.begin(), v.end(), [](auto &a, auto &b) {
                    return a["lan"].template get<std::string>() < b["lan"].template get<std::string>();
                });
            };
            sort(actual["moves"]);
            sort(expected["moves"]);
            require_equal(actual, expected, "Special position");
            for (auto &after : p["afters"])
                require_equal(kernel.call("move", {{"fen", p["fen"]}, {"notation", after["move"]["lan"]}}),
                              after, "Special move");
        }
        {
            auto nodes = kernel.call("layout", fixture["layout"]["data"])["nodes"];
            Json layout = Json::array();
            for (auto &n : nodes)
                layout.push_back({{"id", n["id"]}, {"x", n["x"]}, {"y", n["y"]}});
            require_equal(layout, fixture["layout"]["expected"], "Complex Walker tree");
        }

        size_t games = 0;
        for (auto &p : fixture["pgn"]) {
            auto parsed = kernel.call("parse", {{"pgn", p["input"]}});
            require_equal(parsed, p["games"], "PGN parse " + p["input"].get<std::string>());
            for (size_t i = 0; i < parsed.size(); i++) {
                auto &g = parsed[i];
                for (auto format : {"suffix", "nag", "comment"}) {
                    auto data = g;
                    data["annotationFormat"] = format;
                    require_equal(kernel.call("stringify", data), p["outputs"][i][format],
                                  "PGN " + std::string(format));
                }
                auto nodes = kernel.call("layout", g)["nodes"];
                Json layout = Json::array();
                for (auto &n : nodes)
                    layout.push_back({{"id", n["id"]}, {"x", n["x"]}, {"y", n["y"]}});
                require_equal(layout, p["outputs"][i]["layout"], "Walker coordinates");
                ++games;
            }
        }
        std::cout << "Original oracle: " << count << " positions, 23063 legal moves, " << games
                  << " games passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
