#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <xiangqi/kernel.hpp>
#define REQUIRE(x)                                                                                           \
    do {                                                                                                     \
        if (!(x))                                                                                            \
            throw std::runtime_error("Failed: " #x);                                                         \
    } while (false)
int main() {
    try {
        xqn::Kernel k;
        auto moves = [&](const std::string &fen) { return k.call("position", {{"fen", fen}})["moves"]; };
        auto has = [&](const std::string &fen, const std::string &lan) {
            for (auto &m : moves(fen))
                if (m["lan"] == lan)
                    return true;
            return false;
        };
        auto invalid = [&](const std::string &fen) {
            try {
                k.call("position", {{"fen", fen}});
                return false;
            } catch (...) {
                return true;
            }
        };
        REQUIRE(invalid("4k4/9/9/9/9/9/9/9/9/4K4 w"));
        REQUIRE(invalid("3k5/9/9/9/9/9/9/9/9/A3K4 w"));
        REQUIRE(invalid("3k5/9/9/9/9/9/9/9/9/B3K4 w"));
        REQUIRE(invalid("3k5/9/9/9/9/9/9/9/9/4K4 w - - -1 1"));
        REQUIRE(invalid("3k5/9/9/9/9/9/9/9/9/4K4 w - - 0 0"));
        auto pin = "4k4/9/9/9/4R4/9/9/9/9/4K4 w";
        REQUIRE(!has(pin, "e5a5"));
        REQUIRE(has(pin, "e5e8"));
        REQUIRE(!has(pin, "e5e9"));
        auto cannon = "3k5/9/9/4r4/9/4P4/4C4/9/9/4K4 w";
        REQUIRE(has(cannon, "e3e6"));
        REQUIRE(!has(cannon, "e3e5"));
        REQUIRE(!has("3k5/9/9/4r4/4P4/4P4/4C4/9/9/4K4 w", "e3e6"));
        auto horse = "3k5/9/9/9/9/9/9/4P4/4N4/4K4 w";
        REQUIRE(!has(horse, "e1f3"));
        REQUIRE(!has(horse, "e1d3"));
        REQUIRE(has(horse, "e1g2"));
        auto bishop = "3k5/9/9/9/9/9/9/9/3P5/2B1K4 w";
        REQUIRE(!has(bishop, "c0e2"));
        REQUIRE(has(bishop, "c0a2"));
        REQUIRE(!has("3k5/9/9/9/9/4P4/9/9/9/4K4 w", "e4d4"));
        REQUIRE(has("3k5/9/9/9/4P4/9/9/9/9/4K4 w", "e5d5"));
        auto initial = k.call("initial")["fen"];
        auto retreat = k.call("move", {{"fen", initial}, {"notation", "炮二退一"}});
        REQUIRE(retreat["move"]["lan"] == "h2h1");
        REQUIRE(k.call("move", {{"fen", initial}, {"notation", "C2-1"}}) == retreat);
        REQUIRE(k.call("move", {{"fen", initial}, {"notation", "俥九進一"}})["move"]["lan"] == "a0a1");
        auto pgn = k.call("parse", {{"pgn", R"pgn([Event "a\"b\\c"]
1. 炮二平五 ;中文评论
炮８平５ 2. 马二进三 *)pgn"}});
        REQUIRE(pgn[0]["warnings"].empty());
        REQUIRE(pgn[0]["headers"]["Event"] == "a\"b\\c");
        REQUIRE(pgn[0]["nodes"][1]["comments"][0] == "中文评论");
        REQUIRE(pgn[0]["nodes"].size() == 4);
        auto exported = k.call("stringify", pgn[0]);
        auto reimport = k.call("parse", {{"pgn", exported}});
        REQUIRE(reimport[0]["headers"]["Event"] == pgn[0]["headers"]["Event"]);
        REQUIRE(reimport[0]["warnings"].empty());
        auto bare = k.call("parse", {{"pgn", "3k5/9/9/9/9/9/9/9/9/4K4 b - - 0 14"}});
        REQUIRE(bare[0]["warnings"].empty());
        REQUIRE(bare[0]["fen"] == "3k5/9/9/9/9/9/9/9/9/4K4 b - - 0 14");
        REQUIRE(k.call("parse", {{"pgn", "1. H2-E2 garbage H7-E7"}})[0]["warnings"].size() == 1);
        auto legacy =
            k.call("parse", {{"pgn", "[Result \"?\"]\n1. H2-E2 $01 $999999999999999999 {#a:16} H7-E7"}});
        REQUIRE(legacy[0]["warnings"].empty());
        REQUIRE(!legacy[0]["nodes"][1]["meta"].contains("glyph"));
        REQUIRE(legacy[0]["nodes"][1]["comments"][0] == "#a:16");
        REQUIRE(!legacy[0]["nodes"][2]["meta"].contains("result"));

        {
            // The supported depth must not depend on the Windows thread stack.
            xqn::Json chain = {{"root", "0"},
                               {"fen", initial},
                               {"headers", xqn::Json::object()},
                               {"nodes", xqn::Json::array()}};
            const std::array<std::pair<int, int>, 4> cycle = {{{7, 24}, {88, 69}, {24, 7}, {69, 88}}};
            for (int i = 0; i <= 2048; i++) {
                auto move =
                    i ? xqn::Json{{"from", cycle[(i - 1) % 4].first}, {"to", cycle[(i - 1) % 4].second}}
                      : xqn::Json(nullptr);
                chain["nodes"].push_back(
                    {{"id", std::to_string(i)},
                     {"parent", i ? xqn::Json(std::to_string(i - 1)) : xqn::Json(nullptr)},
                     {"children", i < 2048 ? xqn::Json::array({std::to_string(i + 1)}) : xqn::Json::array()},
                     {"move", move},
                     {"comments", xqn::Json::array()},
                     {"meta", xqn::Json::object()}});
            }
            auto layout = k.call("layout", chain)["nodes"];
            REQUIRE(layout.back()["x"] == 0);
            REQUIRE(layout.back()["y"] == 2048);
            auto text = k.call("stringify", chain).get<std::string>();
            REQUIRE(text.find("1024.") != std::string::npos);
            REQUIRE(k.call("parse", {{"pgn", text}})[0]["nodes"].size() == 2049);
        }

        auto tree = k.call("parse", {{"pgn", "1. H2-E2 H7-E7 *"}})[0];
        auto rejects = [&](xqn::Json data) {
            try {
                k.call("hydrate", data);
                return false;
            } catch (...) {
                return true;
            }
        };
        auto bad = tree;
        bad["nodes"][0]["children"].push_back("node-root");
        REQUIRE(rejects(bad));
        bad = tree;
        bad["nodes"].push_back(bad["nodes"][1]);
        REQUIRE(rejects(bad));
        bad = tree;
        bad["nodes"][0]["children"] = xqn::Json::array();
        REQUIRE(rejects(bad));
        bad = tree;
        bad["nodes"][1]["move"]["from"] = -1;
        REQUIRE(rejects(bad));
        std::cout << "PASS: native rule edge cases, retreats, aliases, Chinese PGN, escaped headers, bare "
                     "FEN, invalid trees\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
