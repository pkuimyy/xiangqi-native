#include <iostream>
#include <oxq/core/reader.hpp>
#include <stdexcept>
#include <xiangqi/document.hpp>
#define REQUIRE(condition)                                                                                   \
    do {                                                                                                     \
        if (!(condition))                                                                                    \
            throw std::runtime_error("Failed: " #condition);                                                 \
    } while (false)
int main() {
    try {
        xqn::Kernel kernel;
        xqn::Document d(kernel);
        REQUIRE(d.position().at("moves").size() == 44);
        REQUIRE(d.position().at("turn") == "w");
        auto n = d.play(25, 22);
        REQUIRE(d.node(n).at("move").at("zh") == "炮二平五");
        REQUIRE(d.modified());
        d.mark_saved();
        REQUIRE(!d.modified());
        d.back();
        auto variation = d.play(7, 24);
        REQUIRE(d.model().move_tree.nodes.front().children.size() == 2);
        d.promote_current();
        REQUIRE(d.model().move_tree.nodes.front().children.front() == variation);
        d.set_comments({"中文评论", "second line"});
        d.set_evaluation({{"score", 35}, {"scoreType", "cp"}, {"depth", 15}, {"bestmove", "h7e7"}});
        auto encoded = d.oxq();
        auto read = oxq::core::read_oxq(encoded);
        REQUIRE(std::holds_alternative<oxq::core::ReaderResult>(read));
        auto reopened =
            xqn::Document::from_model(kernel, std::move(std::get<oxq::core::ReaderResult>(read).game));
        reopened.next();
        REQUIRE(reopened.node(reopened.current()).at("comments").at(0) == "中文评论");
        REQUIRE(reopened.node(reopened.current()).at("meta").at("eval").at("depth") == 15);
        auto imported = xqn::Document::parse_pgn(kernel, d.pgn());
        REQUIRE(imported.size() == 1);
        REQUIRE(imported.front().model().move_tree.nodes.size() == 3);
        auto before = d.model().move_tree.nodes.size();
        d.erase_current();
        REQUIRE(d.model().move_tree.nodes.size() == before - 1);
        REQUIRE(d.undo());
        REQUIRE(d.model().move_tree.nodes.size() == before);
        REQUIRE(d.redo());
        auto games =
            xqn::Document::parse_pgn(kernel, "[Event \"A\"]\n1. H2-E2 H7-E7 *\n\n[Event \"B\"]\n1. H0-G2 *");
        REQUIRE(games.size() == 2);
        bool rejected = false;
        try {
            d.play(0, 89);
        } catch (...) {
            rejected = true;
        }
        REQUIRE(rejected);
        REQUIRE(kernel.call("position", {{"fen", "4k4/3P1P3/9/9/9/9/9/9/9/3K5 b"}}).at("checkmate") == true);

        auto standard = xqn::Document::parse_pgn(
            kernel,
            "[Event \"标准元数据\"]\n[White \"红棋手\"]\n[Black \"黑棋手\"]\n[Result \"1-0\"]\n1. H2-E2 1-0");
        REQUIRE(standard.front().model().metadata.result == oxq::core::GameResult::red_win);
        REQUIRE(standard.front().model().metadata.red_player.name == "红棋手");
        REQUIRE(standard.front().model().metadata.event.name == "标准元数据");
        auto foreign = standard.front().model();
        foreign.move_tree.nodes.front().annotations.push_back(
            {oxq::core::AnnotationKind::source_note, false, "unrecognized source payload", {}, {}});
        auto preserved = xqn::Document::from_model(kernel, std::move(foreign));
        preserved.next();
        preserved.set_comments({"编辑后的评论"});
        REQUIRE(preserved.model().move_tree.nodes.front().annotations.back().text ==
                "unrecognized source payload");
        xqn::Document revisions(kernel);
        revisions.play(25, 22);
        revisions.mark_saved();
        revisions.back();
        revisions.play(7, 24);
        REQUIRE(revisions.modified());
        REQUIRE(revisions.undo());
        REQUIRE(!revisions.modified());

        auto temp = std::filesystem::temp_directory_path() / ("xiangqi-native-" + d.model().uuid.to_string());
        std::filesystem::create_directory(temp);
        auto path = temp / L"中文棋谱.oxq";
        xqn::save_atomic(path, d.oxq());
        auto opened = xqn::open_documents(kernel, path);
        REQUIRE(opened.size() == 1);
        xqn::save_atomic(path, reopened.oxq());
        REQUIRE(xqn::open_documents(kernel, path).front().model().move_tree.nodes.size() == 3);
        std::filesystem::remove(path);
        std::filesystem::remove(temp);
        std::cout
            << "PASS: rules/notation, branch edits, PGN/OXQ metadata, undo/redo, Unicode atomic overwrite\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
