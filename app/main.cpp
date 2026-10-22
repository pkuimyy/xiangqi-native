#include <shobjidl.h>
#include <windows.h>
#undef GetCurrentTime
#include <cmath>
#include <functional>
#include <iomanip>
#include <microsoft.ui.xaml.window.h>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.h>
#include <winrt/base.h>
#include <xiangqi/document.hpp>
#include <xiangqi/uci.hpp>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using Windows::UI::Color;
namespace {
SolidColorBrush brush(unsigned r, unsigned g, unsigned b) {
    return SolidColorBrush(
        Color{255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
}
std::vector<std::byte> bytes(const std::string &text) {
    auto p = reinterpret_cast<const std::byte *>(text.data());
    return {p, p + text.size()};
}
std::wstring piece_name(char c) {
    switch (c) {
    case 'K':
        return L"帅";
    case 'k':
        return L"将";
    case 'A':
        return L"仕";
    case 'a':
        return L"士";
    case 'B':
        return L"相";
    case 'b':
        return L"象";
    case 'N':
    case 'n':
        return L"马";
    case 'R':
    case 'r':
        return L"车";
    case 'C':
    case 'c':
        return L"炮";
    case 'P':
        return L"兵";
    case 'p':
        return L"卒";
    default:
        return L"";
    }
}
std::filesystem::path picker(HWND owner, bool save, const wchar_t *pattern, const wchar_t *extension) {
    com_ptr<IFileDialog> dialog;
    if (save)
        check_hresult(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(dialog.put())));
    else
        check_hresult(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(dialog.put())));
    COMDLG_FILTERSPEC filter{L"棋谱文件", pattern};
    check_hresult(dialog->SetFileTypes(1, &filter));
    check_hresult(dialog->SetDefaultExtension(extension));
    FILEOPENDIALOGOPTIONS flags{};
    check_hresult(dialog->GetOptions(&flags));
    check_hresult(
        dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | (save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST)));
    auto hr = dialog->Show(owner);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        return {};
    check_hresult(hr);
    com_ptr<IShellItem> item;
    check_hresult(dialog->GetResult(item.put()));
    PWSTR path{};
    check_hresult(item->GetDisplayName(SIGDN_FILESYSPATH, &path));
    std::filesystem::path result(path);
    CoTaskMemFree(path);
    return result;
}
struct Workspace : std::enable_shared_from_this<Workspace> {
    xqn::Kernel kernel;
    std::vector<xqn::Document> docs;
    std::vector<std::filesystem::path> paths;
    size_t active{};
    Window window;
    Grid root;
    Canvas board;
    Canvas graph;
    TextBlock status;
    TextBlock turn;
    TextBlock eval;
    TextBox comment;
    TextBox notation;
    ComboBox games;
    Button analyzeButton;
    NumberBox depth;
    HWND hwnd{};
    bool updating{}, flipped{}, allow_close{}, analyzing{};
    int selected = -1;
    std::uint64_t generation{};
    std::filesystem::path enginePath;
    std::shared_ptr<xqn::UciEngine> engine;
    xqn::Document &doc() { return docs.at(active); }
    void cancel() {
        ++generation;
        if (engine)
            engine->stop();
    }
    void run(const std::function<void()> &action) {
        try {
            action();
        } catch (const hresult_error &e) {
            status.Text(L"操作失败：" + e.message());
        } catch (const std::exception &e) {
            status.Text(L"操作失败：" + to_hstring(e.what()));
        }
    }
    Button button(const wchar_t *label, std::function<void()> action) {
        Button b;
        b.Content(box_value(label));
        b.Click([this, action](auto &&, auto &&) { run(action); });
        return b;
    }
    void label(Canvas const &canvas, const std::wstring &text, double x, double y, double size = 18) {
        TextBlock t;
        t.Text(text);
        t.FontSize(size);
        t.Foreground(brush(82, 57, 40));
        Canvas::SetLeft(t, x);
        Canvas::SetTop(t, y);
        canvas.Children().Append(t);
    }
    void line(Canvas const &canvas, double x1, double y1, double x2, double y2, SolidColorBrush const &color,
              double thickness = 1) {
        Line l;
        l.X1(x1);
        l.Y1(y1);
        l.X2(x2);
        l.Y2(y2);
        l.Stroke(color);
        l.StrokeThickness(thickness);
        canvas.Children().Append(l);
    }
    std::wstring comment_text() {
        std::wstring text;
        for (auto &c : doc().node(doc().current()).at("comments")) {
            if (!text.empty())
                text += L"\n";
            text += to_hstring(c.get<std::string>());
        }
        return text;
    }
    void commit_comment() {
        if (comment.Text() != comment_text())
            doc().set_comments({to_string(comment.Text())});
    }
    bool dirty() {
        if (!docs.empty() && comment.Text() != comment_text())
            return true;
        for (auto &d : docs)
            if (d.modified())
                return true;
        return false;
    }
    fire_and_forget confirm_close() {
        auto keep = shared_from_this();
        try {
            ContentDialog dialog;
            dialog.XamlRoot(root.XamlRoot());
            dialog.Title(box_value(L"棋谱尚未保存"));
            dialog.Content(box_value(L"请返回保存需要保留的棋谱，或放弃本次修改并关闭。"));
            dialog.PrimaryButtonText(L"返回保存");
            dialog.SecondaryButtonText(L"放弃修改并关闭");
            dialog.CloseButtonText(L"取消");
            auto result = co_await dialog.ShowAsync();
            if (result == ContentDialogResult::Secondary) {
                allow_close = true;
                window.Close();
            }
        } catch (...) {
        }
    }
    void initialize() {
        docs.emplace_back(kernel);
        paths.emplace_back();
        wchar_t executable[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        auto bundled = std::filesystem::path(executable).parent_path() /
                       L"engines/Pikafish-Windows-x86-64-universal.exe";
        if (std::filesystem::exists(bundled))
            enginePath = bundled;
        window.Title(L"象棋 · Xiangqi Native");
        root.RequestedTheme(ElementTheme::Light);
        root.Background(brush(247, 246, 242));
        root.Padding(Thickness{20, 20, 20, 20});
        root.RowSpacing(12);
        for (auto length : {GridLength{1, GridUnitType::Auto}, GridLength{1, GridUnitType::Auto},
                            GridLength{1, GridUnitType::Star}, GridLength{1, GridUnitType::Auto}}) {
            RowDefinition r;
            r.Height(length);
            root.RowDefinitions().Append(r);
        }
        StackPanel header;
        header.Orientation(Orientation::Horizontal);
        header.Spacing(12);
        TextBlock title;
        title.Text(L"象棋");
        title.FontSize(28);
        title.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
        header.Children().Append(title);
        header.Children().Append(button(L"新建", [this] {
            commit_comment();
            cancel();
            docs.emplace_back(kernel);
            paths.emplace_back();
            active = docs.size() - 1;
            refresh_games();
            refresh();
        }));
        header.Children().Append(button(L"打开…", [this] { open(); }));
        header.Children().Append(button(L"保存 OXQ", [this] { save(); }));
        header.Children().Append(button(L"导出 PGN…", [this] {
            auto p = picker(hwnd, true, L"*.pgn", L"pgn");
            if (!p.empty())
                xqn::save_atomic(p, bytes(doc().pgn()));
        }));
        games.Width(220);
        games.SelectionChanged([this](auto &&, auto &&) {
            if (updating || games.SelectedIndex() < 0)
                return;
            commit_comment();
            cancel();
            active = static_cast<size_t>(games.SelectedIndex());
            selected = -1;
            refresh();
        });
        header.Children().Append(games);
        root.Children().Append(header);
        StackPanel toolbar;
        toolbar.Orientation(Orientation::Horizontal);
        toolbar.Spacing(8);
        Grid::SetRow(toolbar, 1);
        toolbar.Children().Append(button(L"|◀", [this] { navigate([this] { doc().start(); }); }));
        toolbar.Children().Append(button(L"◀", [this] { navigate([this] { doc().back(); }); }));
        toolbar.Children().Append(button(L"▶", [this] { navigate([this] { doc().next(); }); }));
        toolbar.Children().Append(button(L"▶|", [this] { navigate([this] { doc().end(); }); }));
        toolbar.Children().Append(button(L"撤销", [this] { navigate([this] { doc().undo(); }); }));
        toolbar.Children().Append(button(L"重做", [this] { navigate([this] { doc().redo(); }); }));
        toolbar.Children().Append(button(L"翻转", [this] {
            flipped = !flipped;
            draw_board();
        }));
        toolbar.Children().Append(button(L"提升为主线", [this] {
            cancel();
            doc().promote_current();
            refresh();
        }));
        toolbar.Children().Append(button(L"删除分支", [this] {
            cancel();
            doc().erase_current();
            refresh();
        }));
        root.Children().Append(toolbar);
        Grid content;
        content.ColumnSpacing(24);
        Grid::SetRow(content, 2);
        ColumnDefinition left;
        left.Width(GridLength{520, GridUnitType::Pixel});
        content.ColumnDefinitions().Append(left);
        ColumnDefinition right;
        right.Width(GridLength{1, GridUnitType::Star});
        content.ColumnDefinitions().Append(right);
        StackPanel leftPanel;
        leftPanel.Spacing(8);
        turn.FontSize(18);
        leftPanel.Children().Append(turn);
        board.Width(500);
        board.Height(550);
        board.Background(brush(242, 220, 176));
        Viewbox view;
        view.Stretch(Stretch::Uniform);
        view.Child(board);
        view.MaxHeight(570);
        leftPanel.Children().Append(view);
        StackPanel moveRow;
        moveRow.Orientation(Orientation::Horizontal);
        moveRow.Spacing(8);
        notation.Width(300);
        notation.PlaceholderText(L"输入着法，如 H2-E2 / 炮二平五");
        moveRow.Children().Append(notation);
        moveRow.Children().Append(button(L"走子", [this] {
            auto m = kernel.call("move", {{"fen", doc().node(doc().current()).at("fen")},
                                          {"notation", to_string(notation.Text())}});
            navigate([&] { doc().play(m.at("move").at("from"), m.at("move").at("to")); });
            notation.Text(L"");
        }));
        leftPanel.Children().Append(moveRow);
        content.Children().Append(leftPanel);
        Grid detail;
        Grid::SetColumn(detail, 1);
        detail.RowSpacing(8);
        for (auto length : {GridLength{1, GridUnitType::Auto}, GridLength{1, GridUnitType::Star},
                            GridLength{1, GridUnitType::Auto}, GridLength{1, GridUnitType::Auto},
                            GridLength{1, GridUnitType::Auto}}) {
            RowDefinition r;
            r.Height(length);
            detail.RowDefinitions().Append(r);
        }
        StackPanel engineRow;
        engineRow.Orientation(Orientation::Horizontal);
        engineRow.Spacing(8);
        engineRow.Children().Append(button(L"选择皮卡鱼…", [this] {
            if (analyzing)
                throw std::runtime_error("Stop analysis before changing engine");
            auto p = picker(hwnd, false, L"*.exe", L"exe");
            if (!p.empty()) {
                engine.reset();
                enginePath = p;
                status.Text(L"已选择：" + p.filename().wstring());
            }
        }));
        depth.Minimum(1);
        depth.Maximum(30);
        depth.Value(15);
        depth.Width(100);
        engineRow.Children().Append(depth);
        analyzeButton = button(L"分析当前局面", [this] { analyze(); });
        engineRow.Children().Append(analyzeButton);
        engineRow.Children().Append(button(L"停止", [this] {
            cancel();
            status.Text(L"正在停止分析…");
        }));
        detail.Children().Append(engineRow);
        ScrollViewer graphScroll;
        graph.VerticalAlignment(VerticalAlignment::Top);
        graph.HorizontalAlignment(HorizontalAlignment::Left);
        graphScroll.Content(graph);
        graphScroll.VerticalContentAlignment(VerticalAlignment::Top);
        graphScroll.HorizontalContentAlignment(HorizontalAlignment::Left);
        graphScroll.HorizontalScrollBarVisibility(ScrollBarVisibility::Auto);
        graphScroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
        Grid::SetRow(graphScroll, 1);
        detail.Children().Append(graphScroll);
        eval.TextWrapping(TextWrapping::Wrap);
        Grid::SetRow(eval, 2);
        detail.Children().Append(eval);
        comment.AcceptsReturn(true);
        comment.TextWrapping(TextWrapping::Wrap);
        comment.Height(100);
        comment.PlaceholderText(L"当前节点评论");
        Grid::SetRow(comment, 3);
        detail.Children().Append(comment);
        auto saveComment = button(L"保存评论", [this] {
            doc().set_comments({to_string(comment.Text())});
            refresh();
        });
        Grid::SetRow(saveComment, 4);
        detail.Children().Append(saveComment);
        content.Children().Append(detail);
        root.Children().Append(content);
        status.TextWrapping(TextWrapping::Wrap);
        status.FontSize(12);
        Grid::SetRow(status, 3);
        root.Children().Append(status);
        window.Content(root);
        auto native = window.as<IWindowNative>();
        check_hresult(native->get_WindowHandle(&hwnd));
        window.AppWindow().Resize(Windows::Graphics::SizeInt32{1280, 900});
        window.AppWindow().Closing(
            [this](auto &&, Microsoft::UI::Windowing::AppWindowClosingEventArgs const &args) {
                if (!allow_close && dirty()) {
                    args.Cancel(true);
                    confirm_close();
                } else
                    cancel();
            });
        root.KeyDown([this](auto &&, Input::KeyRoutedEventArgs const &e) {
            auto focus = Input::FocusManager::GetFocusedElement(root.XamlRoot());
            if (focus.try_as<TextBox>())
                return;
            switch (e.Key()) {
            case Windows::System::VirtualKey::Left:
                navigate([this] { doc().back(); });
                break;
            case Windows::System::VirtualKey::Right:
                navigate([this] { doc().next(); });
                break;
            case Windows::System::VirtualKey::Home:
                navigate([this] { doc().start(); });
                break;
            case Windows::System::VirtualKey::End:
                navigate([this] { doc().end(); });
                break;
            default:
                return;
            }
            e.Handled(true);
        });
        if (std::wstring(GetCommandLineW()).find(L"--smoke-test") != std::wstring::npos)
            root.Loaded([this](auto &&, auto &&) { smoke_test(); });
        refresh_games();
        refresh();
        window.Activate();
    }
    void refresh_games() {
        updating = true;
        games.Items().Clear();
        for (size_t i = 0; i < docs.size(); ++i)
            games.Items().Append(box_value(L"第 " + std::to_wstring(i + 1) + L" 局"));
        games.SelectedIndex(static_cast<int>(active));
        updating = false;
    }
    void navigate(const std::function<void()> &action) {
        commit_comment();
        cancel();
        action();
        selected = -1;
        refresh();
    }
    void refresh() {
        draw_board();
        draw_graph();
        std::wstring text;
        for (auto &c : doc().node(doc().current()).at("comments")) {
            if (!text.empty())
                text += L"\n";
            text += to_hstring(c.get<std::string>());
        }
        comment.Text(text);
        auto m = doc().node(doc().current()).at("meta");
        if (m.contains("eval")) {

            auto e = m["eval"];
            std::ostringstream score;
            int value = e.value("score", 0);
            if (e.value("scoreType", std::string("cp")) == "mate") {
                score << (value >= 0 ? "+M" : "-M") << std::abs(value);
            } else {
                if (value > 0)
                    score << "+";
                score << std::fixed << std::setprecision(2) << value / 100.0;
            }
            int searchDepth = e.value("depth", 0);
            std::wstring summary = searchDepth > 0 ? L"深度 " + std::to_wstring(searchDepth) : L"已保存评估";
            summary += L" · " + to_hstring(score.str());
            auto best = e.value("bestmove", std::string{});
            if (!best.empty())
                summary += L" · 最佳走法 " + to_hstring(best);
            eval.Text(summary);

        } else
            eval.Text(L"尚无引擎评估");
        window.Title(std::wstring(doc().modified() ? L"● " : L"") + L"象棋 · " +
                     (paths[active].empty() ? L"未命名" : paths[active].filename().wstring()));
        status.Text(L"点击棋子选择，点击落点走子；点击分支树节点切换局面。");
    }

    std::pair<double, double> point(int square) {
        int f = square % 9, r = square / 9;
        if (flipped) {
            f = 8 - f;
            r = 9 - r;
        }
        return {42.0 + f * 52, 40.0 + (9 - r) * 51};
    }
    void arrow(int from, int to, SolidColorBrush color) {
        auto a = point(from), b = point(to);
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
        line(board, a.first, a.second, b.first, b.second, color, 5);
        Microsoft::UI::Xaml::Shapes::Polygon head;
        head.Points().Append(Windows::Foundation::Point{float(b.first), float(b.second)});
        head.Points().Append(Windows::Foundation::Point{float(b.first - dx * 14 - dy * 7),
                                                        float(b.second - dy * 14 + dx * 7)});
        head.Points().Append(Windows::Foundation::Point{float(b.first - dx * 14 + dy * 7),
                                                        float(b.second - dy * 14 - dx * 7)});
        head.Fill(color);
        head.IsHitTestVisible(false);
        board.Children().Append(head);
    }
    void draw_annotations() {
        auto square = [](const std::string &s) -> int {
            if (s.size() != 2 || s[0] < 'a' || s[0] > 'i' || s[1] < '0' || s[1] > '9')
                return -1;
            return s[0] - 'a' + (s[1] - '0') * 9;
        };
        auto meta = doc().node(doc().current()).at("meta");
        if (meta.contains("shapes"))
            for (auto &shape : meta["shapes"]) {
                int from = square(shape.value("orig", std::string{}));
                if (from < 0)
                    continue;
                auto name = shape.value("brush", std::string("g"));
                auto color = name == "r"   ? brush(192, 50, 50)
                             : name == "y" ? brush(194, 153, 31)
                             : name == "b" ? brush(61, 101, 190)
                                           : brush(26, 134, 91);
                color.Opacity(0.75);
                if (shape.contains("dest")) {
                    int to = square(shape.at("dest"));
                    if (to >= 0)
                        arrow(from, to, color);
                } else {
                    auto p = point(from);
                    Microsoft::UI::Xaml::Shapes::Ellipse ring;
                    ring.Width(42);
                    ring.Height(42);
                    ring.Stroke(color);
                    ring.StrokeThickness(4);
                    ring.IsHitTestVisible(false);
                    Canvas::SetLeft(ring, p.first - 21);
                    Canvas::SetTop(ring, p.second - 21);
                    board.Children().Append(ring);
                }
            }
        if (meta.contains("eval")) {
            auto best = meta["eval"].value("bestmove", std::string{});
            if (best.size() == 4) {
                int from = square(best.substr(0, 2)), to = square(best.substr(2, 2));
                if (from >= 0 && to >= 0)
                    arrow(from, to, brush(32, 145, 95));
            }
        }
    }

    void draw_board() {
        board.Children().Clear();
        auto pos = doc().position();
        std::array<char, 90> squares{};
        std::istringstream input(pos.at("fen").get<std::string>());
        std::string fen;
        input >> fen;
        int rank = 9, file = 0;
        for (char c : fen) {
            if (c == '/') {
                --rank;
                file = 0;
            } else if (c >= '1' && c <= '9')
                file += c - '0';
            else
                squares.at(rank * 9 + file++) = c;
        }
        auto grid = brush(123, 87, 49);
        for (int r = 0; r < 10; ++r)
            line(board, 42, 40 + r * 51, 458, 40 + r * 51, grid);
        for (int f = 0; f < 9; ++f) {
            line(board, 42 + f * 52, 40, 42 + f * 52, 244, grid);
            line(board, 42 + f * 52, 295, 42 + f * 52, 499, grid);
        }
        line(board, 42, 244, 42, 295, grid);
        line(board, 458, 244, 458, 295, grid);
        line(board, 198, 40, 302, 142, grid);
        line(board, 302, 40, 198, 142, grid);
        line(board, 198, 397, 302, 499, grid);
        line(board, 302, 397, 198, 499, grid);
        label(board, L"楚 河", 118, 252, 24);
        label(board, L"汉 界", 307, 252, 24);
        draw_annotations();
        std::unordered_set<int> destinations;
        if (selected >= 0)
            for (auto &m : pos.at("moves"))
                if (m.at("from") == selected)
                    destinations.insert(m.at("to").get<int>());
        for (int s = 0; s < 90; ++s) {
            int f = s % 9, r = s / 9;
            if (flipped) {
                f = 8 - f;
                r = 9 - r;
            }
            Button b;
            b.Width(44);
            b.Height(44);
            b.Padding(Thickness{0});
            b.CornerRadius(CornerRadius{22, 22, 22, 22});
            auto c = squares[s];
            b.Content(box_value(c ? piece_name(c) : (destinations.contains(s) ? L"●" : L"")));
            b.FontSize(c ? 27 : 15);
            b.Foreground(c && std::isupper(c) ? brush(170, 38, 36) : brush(35, 41, 38));
            b.Background(c ? brush(255, 242, 205) : SolidColorBrush(Color{0, 0, 0, 0}));
            b.BorderThickness(Thickness{c ? 1.5 : 0, c ? 1.5 : 0, c ? 1.5 : 0, c ? 1.5 : 0});
            b.BorderBrush(s == selected ? brush(24, 129, 98) : grid);
            if (s == selected)
                b.BorderThickness(Thickness{3, 3, 3, 3});
            Canvas::SetLeft(b, 20 + f * 52);
            Canvas::SetTop(b, 18 + (9 - r) * 51);
            b.Click([this, s](auto &&, auto &&) {
                run([this, s] {
                    if (selected == s) {
                        selected = -1;
                        draw_board();
                        return;
                    }
                    auto position = doc().position();
                    if (selected >= 0) {
                        for (auto &m : position.at("moves"))
                            if (m.at("from") == selected && m.at("to") == s) {
                                navigate([this, s] { doc().play(selected, s); });
                                return;
                            }
                    }
                    selected = s;
                    draw_board();
                });
            });
            board.Children().Append(b);
        }
        turn.Text(pos.at("turn") == "w" ? L"红方行棋" : L"黑方行棋");
        if (pos.at("checkmate") == true)
            turn.Text(pos.at("check") == true ? L"将死 · 对局结束" : L"困毙 · 对局结束");
        else if (pos.at("check") == true)
            turn.Text(turn.Text() + L" · 将军");
    }
    void draw_graph() {
        graph.Children().Clear();
        auto data = doc().transport();
        std::unordered_map<std::string, std::pair<double, double>> points;
        double minx = 0, maxx = 0, maxy = 0;
        for (auto &n : data.at("nodes")) {
            auto id = std::stoull(n.at("id").get<std::string>());
            auto &view = doc().node(id);
            double x = view.value("x", 0.0) * 150, y = view.value("y", 0.0) * 50;
            minx = (std::min)(minx, x);
            maxx = (std::max)(maxx, x);
            maxy = (std::max)(maxy, y);
            points[n.at("id")] = {x, y};
        }
        graph.Width((std::max)(500.0, maxx - minx + 140));
        graph.Height((std::max)(260.0, maxy + 70));
        for (auto &n : data.at("nodes"))
            if (!n.at("parent").is_null()) {
                auto a = points.at(n.at("parent")), b = points.at(n.at("id"));
                line(graph, a.first - minx + 70, a.second + 35, b.first - minx + 70, b.second + 35,
                     brush(153, 161, 156), 1.5);
            }
        for (auto &n : data.at("nodes")) {
            auto id = std::stoull(n.at("id").get<std::string>());
            auto p = points.at(n.at("id"));
            auto &view = doc().node(id);
            auto text =
                view.at("move").is_null() ? std::string("开局") : view.at("move").at("zh").get<std::string>();
            Button b =
                button(to_hstring(text).c_str(), [this, id] { navigate([this, id] { doc().select(id); }); });
            b.Width(105);
            b.Height(34);
            b.Padding(Thickness{4, 4, 4, 4});
            if (id == doc().current()) {
                b.Background(brush(24, 115, 90));
                b.Foreground(brush(255, 255, 255));
            }
            Canvas::SetLeft(b, p.first - minx + 18);
            Canvas::SetTop(b, p.second + 18);
            graph.Children().Append(b);
        }
    }
    void open() {
        auto p = picker(hwnd, false, L"*.oxq;*.pgn;*.cbl", L"oxq");
        if (p.empty())
            return;
        commit_comment();
        auto opened = xqn::open_documents(kernel, p);
        if (opened.empty())
            throw std::runtime_error("No games in file");
        cancel();
        active = docs.size();
        for (auto &d : opened) {
            docs.push_back(std::move(d));
            paths.push_back(p.extension() == L".oxq" ? p : std::filesystem::path{});
        }
        refresh_games();
        refresh();
    }
    void save() {
        commit_comment();
        auto p = paths[active];
        if (p.empty())
            p = picker(hwnd, true, L"*.oxq", L"oxq");
        if (p.empty())
            return;
        if (p.extension() != L".oxq")
            p += L".oxq";
        xqn::save_atomic(p, doc().oxq());
        paths[active] = p;
        doc().mark_saved();
        refresh();
    }

    fire_and_forget smoke_test() {
        winrt::apartment_context ui;
        auto keep = shared_from_this();
        wchar_t module[MAX_PATH]{};
        GetModuleFileNameW(nullptr, module, MAX_PATH);
        auto directory = std::filesystem::path(module).parent_path() / L"smoke";
        std::filesystem::create_directories(directory);
        try {
            docs = xqn::Document::parse_pgn(
                kernel, "[Event \"原生迁移验证\"]\n[Red \"红方\"]\n[Black \"黑方\"]\n1. H2-E2! {中炮开局 "
                        "[%eval 0.35] [%cal Gh7e7]} (H0-G2 H9-G7) H7-E7 2. H0-G2 H9-G7 *");
            paths.assign(docs.size(), {});
            active = 0;
            doc().next();
            refresh_games();
            refresh();
            if (board.Children().Size() < 90 || graph.Children().Size() < 7)
                throw std::runtime_error("Native controls did not populate");
            const auto file = directory / L"原生往返.oxq";
            xqn::save_atomic(file, doc().oxq());
            auto loaded = xqn::open_documents(kernel, file);
            if (loaded.size() != 1 ||
                loaded.front().model().move_tree.nodes.size() != doc().model().move_tree.nodes.size())
                throw std::runtime_error("UI document save round-trip failed");
            status.Text(L"原生棋盘、分支树、中文记谱、PGN/OXQ 保存验证通过");
            co_await winrt::resume_after(std::chrono::milliseconds(400));
            // Return to the window thread before accessing any controls.
            co_await ui;
            Microsoft::UI::Xaml::Media::Imaging::RenderTargetBitmap bitmap;
            co_await bitmap.RenderAsync(root);
            auto pixels = co_await bitmap.GetPixelsAsync();
            auto folder =
                co_await Windows::Storage::StorageFolder::GetFolderFromPathAsync(directory.wstring());
            auto image = co_await folder.CreateFileAsync(
                L"native-preview.png", Windows::Storage::CreationCollisionOption::ReplaceExisting);
            auto stream = co_await image.OpenAsync(Windows::Storage::FileAccessMode::ReadWrite);
            auto encoder = co_await Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(
                Windows::Graphics::Imaging::BitmapEncoder::PngEncoderId(), stream);
            std::vector<std::uint8_t> data(pixels.Length());
            Windows::Storage::Streams::DataReader::FromBuffer(pixels).ReadBytes(data);
            encoder.SetPixelData(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
                                 Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied,
                                 bitmap.PixelWidth(), bitmap.PixelHeight(), 96, 96, data);
            co_await encoder.FlushAsync();
            stream.Close();
            xqn::save_atomic(directory / L"result.json",
                             bytes(xqn::Json{{"passed", true},
                                             {"boardControls", board.Children().Size()},
                                             {"graphControls", graph.Children().Size()},
                                             {"gameNodes", doc().model().move_tree.nodes.size()},
                                             {"preview", "native-preview.png"}}
                                       .dump(2)));
        } catch (const hresult_error &e) {
            xqn::save_atomic(directory / L"result.json",
                             bytes(xqn::Json{{"passed", false}, {"error", to_string(e.message())}}.dump(2)));
        } catch (const std::exception &e) {
            xqn::save_atomic(directory / L"result.json",
                             bytes(xqn::Json{{"passed", false}, {"error", e.what()}}.dump(2)));
        }
        allow_close = true;
        window.Close();
    }

    void analyze() {
        if (analyzing)
            throw std::runtime_error("Analysis already running");
        if (enginePath.empty()) {
            enginePath = picker(hwnd, false, L"*.exe", L"exe");
            if (enginePath.empty())
                return;
        }
        if (!engine)
            engine = std::make_shared<xqn::UciEngine>(enginePath);
        auto service = engine;
        auto weak = weak_from_this();
        auto queue = window.DispatcherQueue();
        auto fen = doc().node(doc().current()).at("fen").get<std::string>();
        auto node = doc().current();
        auto revision = ++generation;
        auto index = active;
        int limit = static_cast<int>(depth.Value());
        analyzing = true;
        analyzeButton.IsEnabled(false);
        status.Text(L"皮卡鱼正在分析…");
        std::thread([weak, queue, service, fen, node, revision, index, limit] {
            xqn::Json result;
            std::string error;
            try {
                result = xqn::red_view(service->analyze(fen, limit), fen);
            } catch (const std::exception &e) {
                error = e.what();
            }
            queue.TryEnqueue([weak, result, error, node, revision, index] {
                if (auto self = weak.lock()) {
                    self->analyzing = false;
                    self->analyzeButton.IsEnabled(true);
                    if (self->generation != revision || self->active != index ||
                        self->doc().current() != node) {
                        self->status.Text(L"分析已停止");
                        return;
                    }
                    self->run([&] {
                        if (!error.empty()) {
                            self->engine.reset();
                            throw std::runtime_error(error);
                        }
                        self->commit_comment();
                        self->doc().set_evaluation(result);
                        self->refresh();
                    });
                }
            });
        }).detach();
    }
};

void startup_error(const hstring &message) {
    if (std::wstring(GetCommandLineW()).find(L"--smoke-test") == std::wstring::npos) {
        MessageBoxW(nullptr, message.c_str(), L"象棋启动失败", MB_ICONERROR);
        return;
    }
    wchar_t module[MAX_PATH]{};
    GetModuleFileNameW(nullptr, module, MAX_PATH);
    auto folder = std::filesystem::path(module).parent_path() / L"smoke";
    std::filesystem::create_directories(folder);
    xqn::save_atomic(folder / L"result.json",
                     bytes(xqn::Json{{"passed", false}, {"startupError", to_string(message)}}.dump(2)));
}

struct App : ApplicationT<App, Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> {
    Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider metadata;
    Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(Windows::UI::Xaml::Interop::TypeName const &type) {
        return metadata.GetXamlType(type);
    }
    Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(hstring const &type) {
        return metadata.GetXamlType(type);
    }
    com_array<Microsoft::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions() {
        return metadata.GetXmlnsDefinitions();
    }
    std::shared_ptr<Workspace> workspace;
    void OnLaunched(LaunchActivatedEventArgs const &) {
        try {
            Resources().MergedDictionaries().Append(XamlControlsResources());
            workspace = std::make_shared<Workspace>();
            workspace->initialize();
        } catch (const std::exception &e) {
            startup_error(to_hstring(e.what()));
            Exit();
        } catch (const hresult_error &e) {
            startup_error(e.message());
            Exit();
        }
    }
};
} // namespace
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    init_apartment(apartment_type::single_threaded);
    Application::Start([](auto &&) { make<App>(); });
    return 0;
}
