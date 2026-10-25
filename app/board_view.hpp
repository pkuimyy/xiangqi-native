#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.h>
#include <xiangqi/kernel.hpp>

namespace xqn::ui {
namespace xaml = winrt::Microsoft::UI::Xaml;
namespace controls = winrt::Microsoft::UI::Xaml::Controls;
namespace media = winrt::Microsoft::UI::Xaml::Media;
namespace drawing = winrt::Microsoft::UI::Xaml::Shapes;

// Retained board visuals. A position change never tears down the board subtree.
class BoardView {
  public:
    using PieceFactory = std::function<controls::Canvas(char)>;
    using PieceName = std::function<winrt::hstring(char)>;
    BoardView(controls::Canvas const &root, controls::ControlTemplate const &squareTemplate,
              PieceFactory makePiece, PieceName pieceName, std::function<void(int)> click)
        : root_(root), makePiece_(std::move(makePiece)), pieceName_(std::move(pieceName)) {
        pieceAt_.fill(-1);
        for (auto layer : {background_, cells_, piecesLayer_, hints_, annotations_}) {
            layer.Width(450);
            layer.Height(500);
            layer.ChildrenTransitions(media::Animation::TransitionCollection{});
            root_.Children().Append(layer);
        }
        background_.IsHitTestVisible(false);
        piecesLayer_.IsHitTestVisible(false);
        hints_.IsHitTestVisible(false);
        annotations_.IsHitTestVisible(false);
        grid_.Width(450);
        grid_.Height(500);
        grid_.Source(media::Imaging::SvgImageSource(
            winrt::Windows::Foundation::Uri(L"ms-appx:///assets/board-grid.svg")));
        background_.Children().Append(grid_);
        rivers_.push_back(label(L"楚 河", 72, 229, 28));
        rivers_.push_back(label(L"漢 界", 322, 229, 28));
        for (int f = 0; f < 9; ++f) {
            top_.push_back(label(L"", 20 + f * 50, 0, 12));
            bottom_.push_back(label(L"", 19 + f * 50, 481, 12));
        }
        buttons_.reserve(90);
        dots_.reserve(90);
        for (int s = 0; s < 90; ++s) {
            controls::Button b;
            b.Template(squareTemplate);
            b.UseSystemFocusVisuals(true);
            b.Transitions(media::Animation::TransitionCollection{});
            b.ContentTransitions(media::Animation::TransitionCollection{});
            b.Width(48);
            b.Height(48);
            b.Padding(xaml::Thickness{0});
            b.CornerRadius(xaml::CornerRadius{24});
            b.Background(color(0, 0, 0, 0));
            b.BorderThickness(xaml::Thickness{0});
            b.BorderBrush(color(126, 111, 161));
            b.HorizontalContentAlignment(xaml::HorizontalAlignment::Center);
            b.VerticalContentAlignment(xaml::VerticalAlignment::Center);
            b.Click([click, s](auto &&, auto &&) { click(s); });
            cells_.Children().Append(b);
            buttons_.push_back(b);
            drawing::Ellipse dot;
            dot.Width(12);
            dot.Height(12);
            dot.Fill(color(30, 120, 100));
            dot.Opacity(.6);
            dot.Visibility(xaml::Visibility::Collapsed);
            hints_.Children().Append(dot);
            dots_.push_back(dot);
        }
    }

    void render(Json const &position, Json const &node, int selected, bool flipped, bool bamboo) {
        bool geometryChanged = !ready_ || flipped != flipped_ || bamboo != bamboo_;
        if (!ready_ || bamboo != bamboo_)
            update_background(bamboo);
        if (geometryChanged)
            update_geometry(flipped, bamboo);
        auto next = placement(position.at("fen").get<std::string>());

        // Update square ownership without detaching any SVG from its fixed visual layer.
        for (int s = 0; s < 90; ++s) {
            if (next[s] == displayed_[s])
                continue;
            if (pieceAt_[s] >= 0) {
                pieces_[pieceAt_[s]].square = -1;
                pieceAt_[s] = -1;
            }
        }
        for (int s = 0; s < 90; ++s) {
            if (next[s] == displayed_[s])
                continue;
            if (next[s]) {
                int index = spare_piece(next[s]);
                pieces_[index].square = s;
                pieceAt_[s] = index;
            }
            auto name = next[s] ? pieceName_(next[s]) : winrt::hstring(L"空位");
            xaml::Automation::AutomationProperties::SetName(buttons_[s],
                                                            name + L" " + winrt::to_hstring(algebraic(s)));
        }
        // Initially empty squares also need accessibility labels.
        if (!ready_)
            for (int s = 0; s < 90; ++s)
                if (!next[s])
                    xaml::Automation::AutomationProperties::SetName(
                        buttons_[s], L"空位 " + winrt::to_hstring(algebraic(s)));
        displayed_ = next;

        for (auto &piece : pieces_) {
            bool visible = piece.square >= 0;
            if (visible && (piece.positionedSquare != piece.square || geometryChanged)) {
                auto p = point(piece.square, flipped, bamboo);
                controls::Canvas::SetLeft(piece.frame, p.first - 22.5);
                controls::Canvas::SetTop(piece.frame, p.second - 22.5);
                piece.positionedSquare = piece.square;
            }
            bool enlarge = visible && piece.square == selected;
            if (piece.selected != enlarge) {
                piece.scale.ScaleX(enlarge ? 1.10 : 1.0);
                piece.scale.ScaleY(enlarge ? 1.10 : 1.0);
                piece.selected = enlarge;
            }
            if (piece.visible != visible) {
                piece.frame.Visibility(visible ? xaml::Visibility::Visible : xaml::Visibility::Collapsed);
                piece.visible = visible;
            }
        }

        std::array<bool, 90> legal{};
        if (selected >= 0)
            for (auto &move : position.at("moves"))
                if (move.at("from") == selected)
                    legal.at(move.at("to").get<int>()) = true;
        int last = node.at("move").is_null() ? -1 : node.at("move").at("to").get<int>();
        for (int s = 0; s < 90; ++s) {
            bool mark = s == last && s != selected;
            if (mark != marked_[s]) {
                buttons_[s].BorderThickness(xaml::Thickness{mark ? 1.5 : 0.0});
                marked_[s] = mark;
            }
            bool hint = legal[s] && !next[s];
            if (hint != hinted_[s]) {
                dots_[s].Visibility(hint ? xaml::Visibility::Visible : xaml::Visibility::Collapsed);
                hinted_[s] = hint;
            }
        }

        const auto &meta = node.at("meta");
        auto shapes = meta.value("shapes", Json::array());
        auto best = meta.contains("eval") ? meta["eval"].value("bestmove", std::string{}) : std::string{};
        std::string key = Json{{"shapes", shapes}, {"bestmove", best}}.dump();
        if (!ready_ || geometryChanged || key != annotationKey_) {
            annotations_.Children().Clear();
            draw_annotations(shapes, best, flipped, bamboo);
            annotationKey_ = std::move(key);
        }
        ready_ = true;
        flipped_ = flipped;
        bamboo_ = bamboo;
    }

    controls::Button square(int s) const { return buttons_.at(s); }
    controls::Canvas piece(int s) const {
        int index = pieceAt_.at(s);
        return index >= 0 ? pieces_[index].frame : controls::Canvas{nullptr};
    }
    controls::Image grid_image() const { return grid_; }
    std::size_t square_count() const { return buttons_.size(); }
    std::size_t piece_visual_count() const { return pieces_.size(); }
    std::uint32_t layer_count() const { return root_.Children().Size(); }

  private:
    struct Piece {
        char symbol;
        controls::Canvas frame;
        media::ScaleTransform scale;
        int square = -1;
        bool selected = false;
        bool visible = false;
        int positionedSquare = -1;
    };
    controls::Canvas root_, background_, cells_, piecesLayer_, hints_, annotations_;
    controls::Image grid_;
    std::vector<controls::TextBlock> rivers_, top_, bottom_;
    std::vector<controls::Button> buttons_;
    std::vector<drawing::Ellipse> dots_;
    std::vector<Piece> pieces_;
    PieceFactory makePiece_;
    PieceName pieceName_;
    std::array<char, 90> displayed_{};
    std::array<int, 90> pieceAt_;
    std::array<bool, 90> marked_{}, hinted_{};
    media::ImageBrush bambooBrush_{nullptr};
    bool ready_ = false, flipped_ = false, bamboo_ = false;
    std::string annotationKey_;

    static media::SolidColorBrush color(unsigned r, unsigned g, unsigned b, unsigned a = 255) {
        return media::SolidColorBrush(
            winrt::Windows::UI::Color{static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(r),
                                      static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)});
    }
    static std::string algebraic(int s) {
        return std::string(1, static_cast<char>('a' + s % 9)) + static_cast<char>('0' + s / 9);
    }
    static int square_index(std::string const &s) {
        if (s.size() != 2 || s[0] < 'a' || s[0] > 'i' || s[1] < '0' || s[1] > '9')
            return -1;
        return s[0] - 'a' + (s[1] - '0') * 9;
    }
    static std::array<char, 90> placement(std::string const &fen) {
        std::array<char, 90> squares{};
        int rank = 9, file = 0;
        for (char c : fen) {
            if (c == ' ')
                break;
            if (c == '/') {
                --rank;
                file = 0;
            } else if (c >= '1' && c <= '9')
                file += c - '0';
            else
                squares.at(rank * 9 + file++) = c;
        }
        return squares;
    }
    controls::TextBlock label(std::wstring const &text, double x, double y, double size) {
        controls::TextBlock result;
        result.Text(text);
        result.FontFamily(media::FontFamily(L"SimSun"));
        result.FontSize(size);
        controls::Canvas::SetLeft(result, x);
        controls::Canvas::SetTop(result, y);
        background_.Children().Append(result);
        return result;
    }
    void update_background(bool bamboo) {
        if (bamboo) {
            if (!bambooBrush_) {
                bambooBrush_ = media::ImageBrush{};
                bambooBrush_.ImageSource(media::Imaging::BitmapImage(
                    winrt::Windows::Foundation::Uri(L"ms-appx:///assets/bamboo.jpg")));
                bambooBrush_.Stretch(media::Stretch::Fill);
            }
            root_.Background(bambooBrush_);
        } else
            root_.Background(color(245, 246, 248));
        auto visible = bamboo ? xaml::Visibility::Collapsed : xaml::Visibility::Visible;
        grid_.Visibility(visible);
        auto ink = bamboo ? color(48, 76, 50) : color(99, 110, 125);
        for (auto label : rivers_) {
            label.Visibility(visible);
            label.Foreground(ink);
        }
        for (auto label : top_)
            label.Foreground(ink);
        for (auto label : bottom_)
            label.Foreground(ink);
    }
    static std::pair<double, double> point(int square, bool flipped, bool bamboo) {
        int file = square % 9, rank = square / 9;
        if (flipped) {
            file = 8 - file;
            rank = 9 - rank;
        }
        if (bamboo)
            return {450.0 * (32.0 + 58.875 * file) / 537.0, 500.0 * (32.0 + 59.222222 * (9 - rank)) / 600.0};
        return {25.0 + file * 50, 25.0 + (9 - rank) * 50};
    }
    void update_geometry(bool flipped, bool bamboo) {
        const std::wstring numbers = L"九八七六五四三二一";
        for (int f = 0; f < 9; ++f) {
            top_[f].Text(std::to_wstring(flipped ? 9 - f : f + 1));
            bottom_[f].Text(std::wstring(1, numbers[flipped ? 8 - f : f]));
        }
        for (int s = 0; s < 90; ++s) {
            auto p = point(s, flipped, bamboo);
            controls::Canvas::SetLeft(buttons_[s], p.first - 24);
            controls::Canvas::SetTop(buttons_[s], p.second - 24);
            controls::Canvas::SetLeft(dots_[s], p.first - 6);
            controls::Canvas::SetTop(dots_[s], p.second - 6);
        }
    }
    int spare_piece(char symbol) {
        for (int i = 0; i < static_cast<int>(pieces_.size()); ++i)
            if (pieces_[i].square < 0 && pieces_[i].symbol == symbol)
                return i;
        auto frame = makePiece_(symbol);
        auto scale = frame.RenderTransform().as<media::ScaleTransform>();
        frame.Visibility(xaml::Visibility::Collapsed);
        piecesLayer_.Children().Append(frame);
        pieces_.push_back({symbol, frame, scale});
        return static_cast<int>(pieces_.size()) - 1;
    }
    void line(double x1, double y1, double x2, double y2, media::SolidColorBrush const &ink, double width) {
        drawing::Line line;
        line.X1(x1);
        line.Y1(y1);
        line.X2(x2);
        line.Y2(y2);
        line.Stroke(ink);
        line.StrokeThickness(width);
        annotations_.Children().Append(line);
    }
    void arrow(int from, int to, media::SolidColorBrush const &ink, bool flipped, bool bamboo) {
        auto a = point(from, flipped, bamboo), b = point(to, flipped, bamboo);
        double dx = b.first - a.first, dy = b.second - a.second;
        double length = std::sqrt(dx * dx + dy * dy);
        if (length < 1)
            return;
        dx /= length;
        dy /= length;
        a.first += dx * 20;
        a.second += dy * 20;
        b.first -= dx * 13;
        b.second -= dy * 13;
        line(a.first, a.second, b.first, b.second, ink, 5);
        drawing::Polygon head;
        head.Points().Append(winrt::Windows::Foundation::Point{float(b.first), float(b.second)});
        head.Points().Append(winrt::Windows::Foundation::Point{float(b.first - dx * 14 - dy * 7),
                                                               float(b.second - dy * 14 + dx * 7)});
        head.Points().Append(winrt::Windows::Foundation::Point{float(b.first - dx * 14 + dy * 7),
                                                               float(b.second - dy * 14 - dx * 7)});
        head.Fill(ink);
        annotations_.Children().Append(head);
    }
    void draw_annotations(Json const &shapes, std::string const &best, bool flipped, bool bamboo) {
        for (auto const &shape : shapes) {
            int from = square_index(shape.value("orig", std::string{}));
            if (from < 0)
                continue;
            auto name = shape.value("brush", std::string("g"));
            auto ink = name == "r"   ? color(192, 50, 50)
                       : name == "y" ? color(194, 153, 31)
                       : name == "b" ? color(61, 101, 190)
                                     : color(26, 134, 91);
            ink.Opacity(.75);
            if (shape.contains("dest")) {
                int to = square_index(shape.at("dest").get<std::string>());
                if (to >= 0)
                    arrow(from, to, ink, flipped, bamboo);
            } else {
                auto p = point(from, flipped, bamboo);
                drawing::Ellipse ring;
                ring.Width(42);
                ring.Height(42);
                ring.Stroke(ink);
                ring.StrokeThickness(4);
                controls::Canvas::SetLeft(ring, p.first - 21);
                controls::Canvas::SetTop(ring, p.second - 21);
                annotations_.Children().Append(ring);
            }
        }
        if (best.size() == 4) {
            int from = square_index(best.substr(0, 2)), to = square_index(best.substr(2, 2));
            if (from >= 0 && to >= 0)
                arrow(from, to, color(32, 145, 95), flipped, bamboo);
        }
    }
};
} // namespace xqn::ui
