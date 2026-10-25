#include <shobjidl.h>
#include <windows.h>
#undef GetCurrentTime
#include "board_view.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <microsoft.ui.xaml.window.h>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Provider.h>
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
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
// User-provided OXDF-editor SVGs contain path glyphs, not font-dependent text.
using Microsoft::UI::Xaml::Media::Imaging::SvgImageSource;
std::unordered_map<char, SvgImageSource> pieceArt;
std::unordered_set<char> loadedPieceArt, failedPieceArt;
std::filesystem::path asset_root() {
    wchar_t module[MAX_PATH]{};
    GetModuleFileNameW(nullptr, module, MAX_PATH);
    return std::filesystem::path(module).parent_path() / L"assets";
}
void load_piece_art() {
    const std::map<char, std::wstring> files = {
        {'K', L"red_king"},     {'k', L"black_king"},     {'A', L"red_advisor"}, {'a', L"black_advisor"},
        {'B', L"red_elephant"}, {'b', L"black_elephant"}, {'N', L"red_horse"},   {'n', L"black_horse"},
        {'R', L"red_rook"},     {'r', L"black_rook"},     {'C', L"red_cannon"},  {'c', L"black_cannon"},
        {'P', L"red_pawn"},     {'p', L"black_pawn"}};
    for (auto &[piece, name] : files) {
        if (!std::filesystem::exists(asset_root() / L"pieces" / (name + L".svg")))
            throw std::runtime_error("Missing OXDF-editor piece resource");
        SvgImageSource source;
        source.Opened([piece](auto &&, auto &&) { loadedPieceArt.insert(piece); });
        source.OpenFailed([piece](auto &&, auto &&) { failedPieceArt.insert(piece); });
        source.UriSource(Windows::Foundation::Uri(L"ms-appx:///assets/pieces/" + name + L".svg"));
        pieceArt.emplace(piece, source);
    }
}
Canvas piece_visual(char c, bool isSelected = false) {
    // Original 96x96 SVGs have a 58px disc centred at 48,48.
    // Crop only the transparent margin so the unchanged artwork fills a board piece.
    Canvas frame;
    frame.Width(45);
    frame.Height(45);
    frame.IsHitTestVisible(false);
    // Selection grows only the artwork around its fixed centre, without a fade or outline.
    frame.RenderTransformOrigin(Windows::Foundation::Point{0.5f, 0.5f});
    ScaleTransform selectionScale;
    selectionScale.ScaleX(isSelected ? 1.10 : 1.0);
    selectionScale.ScaleY(isSelected ? 1.10 : 1.0);
    frame.RenderTransform(selectionScale);
    RectangleGeometry clip;
    clip.Rect(Windows::Foundation::Rect{0, 0, 45, 45});
    frame.Clip(clip);
    Image image;
    constexpr double size = 45.0 * 96.0 / 62.0;
    image.Width(size);
    image.Height(size);
    image.Source(pieceArt.at(c));
    Canvas::SetLeft(image, (45 - size) / 2);
    Canvas::SetTop(image, (45 - size) / 2);
    frame.Children().Append(image);
    return frame;
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
    std::unique_ptr<xqn::ui::BoardView> boardView;
    ControlTemplate squareTemplate{nullptr};
    Grid evaluationStrip;
    bool bamboo = false;
    Canvas graph;
    StackPanel moveList;
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
        t.IsHitTestVisible(false);
        t.FontSize(size);
        t.Foreground(bamboo ? brush(48, 76, 50) : brush(99, 110, 125));
        t.FontFamily(FontFamily(L"SimSun"));
        Canvas::SetLeft(t, x);
        Canvas::SetTop(t, y);
        canvas.Children().Append(t);
    }
    void line(Canvas const &canvas, double x1, double y1, double x2, double y2, SolidColorBrush const &color,
              double thickness = 1) {
        Line l;
        l.IsHitTestVisible(false);
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
        squareTemplate = Markup::XamlReader::Load(LR"XAML(
<ControlTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                 TargetType="Button">
  <ContentPresenter Content="{TemplateBinding Content}"
                    ContentTemplate="{TemplateBinding ContentTemplate}"
                    Padding="{TemplateBinding Padding}"
                    HorizontalContentAlignment="{TemplateBinding HorizontalContentAlignment}"
                    VerticalContentAlignment="{TemplateBinding VerticalContentAlignment}"
                    Background="{TemplateBinding Background}"
                    BorderBrush="{TemplateBinding BorderBrush}"
                    BorderThickness="{TemplateBinding BorderThickness}"
                    CornerRadius="{TemplateBinding CornerRadius}" />
</ControlTemplate>)XAML")
                             .as<ControlTemplate>();

        load_piece_art();
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
            commit_comment();
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
            commit_comment();
            doc().promote_current();
            refresh();
        }));
        toolbar.Children().Append(button(L"删除分支", [this] {
            cancel();
            commit_comment();
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
        board.ChildrenTransitions(Microsoft::UI::Xaml::Media::Animation::TransitionCollection{});
        board.Width(450);
        board.Height(500);
        board.Background(brush(245, 246, 248));
        Viewbox view;
        view.Stretch(Stretch::Uniform);
        view.Child(board);
        view.MaxHeight(570);
        Grid boardArea;
        boardArea.ColumnSpacing(8);
        for (auto width : {GridLength{8, GridUnitType::Pixel}, GridLength{1, GridUnitType::Star},
                           GridLength{36, GridUnitType::Pixel}}) {
            ColumnDefinition col;
            col.Width(width);
            boardArea.ColumnDefinitions().Append(col);
        }
        for (int i = 0; i < 2; i++) {
            RowDefinition r;
            r.Height(GridLength{1, GridUnitType::Star});
            evaluationStrip.RowDefinitions().Append(r);
        }
        Border blackFill;
        blackFill.Background(brush(27, 35, 49));
        evaluationStrip.Children().Append(blackFill);
        Border redFill;
        redFill.Background(brush(253, 248, 239));
        Grid::SetRow(redFill, 1);
        evaluationStrip.Children().Append(redFill);
        Border evaluationCard;
        evaluationCard.CornerRadius(CornerRadius{4});
        evaluationCard.BorderBrush(brush(206, 210, 218));
        evaluationCard.BorderThickness(Thickness{1});
        evaluationCard.Child(evaluationStrip);
        boardArea.Children().Append(evaluationCard);
        Border boardFrame;
        boardFrame.CornerRadius(CornerRadius{8});
        boardFrame.Child(view);
        Grid::SetColumn(boardFrame, 1);
        boardArea.Children().Append(boardFrame);
        StackPanel rail;
        rail.Spacing(6);
        rail.VerticalAlignment(VerticalAlignment::Center);
        auto railButton = [&](Symbol icon, const wchar_t *tip, std::function<void()> action) {
            auto b = button(L"", action);
            b.Content(SymbolIcon(icon));
            b.Width(34);
            b.Height(34);
            b.MinWidth(0);
            b.MinHeight(0);
            b.Padding(Thickness{0});
            b.CornerRadius(CornerRadius{17});
            ToolTipService::SetToolTip(b, box_value(tip));
            rail.Children().Append(b);
        };
        railButton(Symbol::Refresh, L"翻转棋盘", [this] {
            flipped = !flipped;
            draw_board();
            refresh_evaluation_strip();
        });
        railButton(Symbol::Undo, L"撤销", [this] { navigate([this] { doc().undo(); }); });
        railButton(Symbol::Redo, L"重做", [this] { navigate([this] { doc().redo(); }); });
        railButton(Symbol::Copy, L"复制 FEN", [this] {
            Windows::ApplicationModel::DataTransfer::DataPackage data;
            data.SetText(to_hstring(doc().node(doc().current()).at("fen").get<std::string>()));
            Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(data);
            status.Text(L"已复制当前 FEN");
        });
        Grid::SetColumn(rail, 2);
        boardArea.Children().Append(rail);
        leftPanel.Children().Append(boardArea);
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
        Grid study;
        study.ColumnSpacing(12);
        ColumnDefinition treeColumn;
        treeColumn.Width(GridLength{1, GridUnitType::Star});
        study.ColumnDefinitions().Append(treeColumn);
        ColumnDefinition listColumn;
        listColumn.Width(GridLength{238, GridUnitType::Pixel});
        study.ColumnDefinitions().Append(listColumn);
        Border treeCard;
        treeCard.Background(brush(255, 255, 255));
        treeCard.CornerRadius(CornerRadius{8});
        treeCard.Child(graphScroll);
        study.Children().Append(treeCard);
        ScrollViewer moveScroll;
        moveScroll.Content(moveList);
        moveScroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
        Border listCard;
        listCard.Background(brush(255, 255, 255));
        listCard.CornerRadius(CornerRadius{8});
        listCard.Child(moveScroll);
        Grid::SetColumn(listCard, 1);
        study.Children().Append(listCard);
        Grid::SetRow(study, 1);
        detail.Children().Append(study);
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
    void refresh_evaluation_strip() {
        double redShare = .5;
        auto meta = doc().node(doc().current()).at("meta");
        if (meta.contains("eval")) {
            auto ev = meta["eval"];
            int score = ev.value("score", 0);
            redShare = ev.value("scoreType", std::string("cp")) == "mate"
                           ? (score > 0   ? .99
                              : score < 0 ? .01
                                          : .5)
                           : 1.0 / (1.0 + std::exp(-.00368208 * score));
        }
        redShare = (std::clamp)(redShare, .01, .99);
        evaluationStrip.RowDefinitions().GetAt(0).Height(
            GridLength{flipped ? redShare : 1 - redShare, GridUnitType::Star});
        evaluationStrip.RowDefinitions().GetAt(1).Height(
            GridLength{flipped ? 1 - redShare : redShare, GridUnitType::Star});
        evaluationStrip.Children().GetAt(0).as<Border>().Background(flipped ? brush(253, 248, 239)
                                                                            : brush(27, 35, 49));
        evaluationStrip.Children().GetAt(1).as<Border>().Background(flipped ? brush(27, 35, 49)
                                                                            : brush(253, 248, 239));
    }

    void refresh() {
        refresh_evaluation_strip();
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

    void draw_board() {
        if (!boardView) {
            boardView = std::make_unique<xqn::ui::BoardView>(
                board, squareTemplate, [](char c) { return piece_visual(c); },
                [](char c) { return hstring(piece_name(c)); },
                [this](int s) {
                    run([this, s] {
                        if (selected == s) {
                            selected = -1;
                            draw_board();
                            return;
                        }
                        auto position = doc().position();
                        if (selected >= 0)
                            for (auto &move : position.at("moves"))
                                if (move.at("from") == selected && move.at("to") == s) {
                                    navigate([this, s] { doc().play(selected, s); });
                                    return;
                                }
                        selected = s;
                        draw_board();
                    });
                });
        }
        auto pos = doc().position();
        boardView->render(pos, doc().node(doc().current()), selected, flipped, bamboo);
        turn.Text(pos.at("turn") == "w" ? L"红方行棋" : L"黑方行棋");
        if (pos.at("checkmate") == true)
            turn.Text(pos.at("check") == true ? L"将死 · 对局结束" : L"困毙 · 对局结束");
        else if (pos.at("check") == true)
            turn.Text(turn.Text() + L" · 将军");
    }

    static std::string xqn_square(int s) { return std::string(1, char('a' + s % 9)) + char('0' + s / 9); }

    void draw_graph() {
        graph.Children().Clear();
        moveList.Children().Clear();
        auto data = doc().transport();
        std::unordered_map<std::string, std::pair<double, double>> points;
        std::unordered_map<std::string, xqn::Json> byid;
        std::unordered_set<std::string> path;
        auto cursor = std::to_string(doc().current());
        for (auto &n : data.at("nodes"))
            byid[n.at("id").get<std::string>()] = doc().node(std::stoull(n.at("id").get<std::string>()));
        for (auto id = cursor;;) {
            path.insert(id);
            auto &p = byid.at(id).at("parent");
            if (p.is_null())
                break;
            id = p.get<std::string>();
        }
        double minx = 0, maxx = 0, maxy = 0;
        for (auto &n : data.at("nodes")) {
            auto id = n.at("id").get<std::string>();
            auto &v = doc().node(std::stoull(id));
            double x = v.value("x", 0.0) * 24, y = v.value("y", 0.0) * 30;
            minx = (std::min)(minx, x);
            maxx = (std::max)(maxx, x);
            maxy = (std::max)(maxy, y);
            points[id] = {x, y};
        }
        graph.Width((std::max)(220.0, maxx - minx + 80));
        graph.Height((std::max)(270.0, maxy + 60));
        for (auto &n : data.at("nodes"))
            if (!n.at("parent").is_null()) {
                auto id = n.at("id").get<std::string>();
                auto a = points.at(n.at("parent")), b = points.at(id);
                auto color = path.contains(id) ? brush(108, 113, 125) : brush(192, 197, 204);
                double ax = a.first - minx + 40, ay = a.second + 26, bx = b.first - minx + 40,
                       by = b.second + 26;
                line(graph, ax, ay, ax, by - 15, color, 1.5);
                line(graph, ax, by - 15, bx, by - 15, color, 1.5);
                line(graph, bx, by - 15, bx, by, color, 1.5);
            }
        for (auto &n : data.at("nodes")) {
            auto id = n.at("id").get<std::string>();
            auto p = points.at(id);
            auto &view = doc().node(std::stoull(id));
            bool start = view.at("move").is_null();
            auto notation =
                start ? std::wstring(L"始") : std::wstring(to_hstring(view["move"]["zh"].get<std::string>()));
            bool prefix = !start && !notation.empty() &&
                          std::wstring(L"前后中一二三四五").find(notation[0]) != std::wstring::npos;
            auto text = notation.substr(prefix ? 1 : 0, 1);
            auto b =
                button(text.c_str(), [this, id] { navigate([this, id] { doc().select(std::stoull(id)); }); });
            b.Width(24);
            b.Height(24);
            b.MinWidth(0);
            b.MinHeight(0);
            b.Padding(Thickness{0});
            b.CornerRadius(CornerRadius{5});
            b.FontSize(14);
            b.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
            b.BorderThickness(Thickness{0});
            bool red = false;
            if (!n["parent"].is_null())
                red = byid.at(n["parent"])["fen"].get<std::string>().find(" w ") != std::string::npos;
            b.Background(id == cursor ? brush(124, 58, 237)
                         : start      ? brush(90, 100, 115)
                         : red        ? brush(134, 24, 24)
                                      : brush(10, 28, 58));
            b.Foreground(brush(255, 255, 255));
            b.Opacity(path.contains(id) || start ? 1.0 : .55);
            if (!start)
                ToolTipService::SetToolTip(b, box_value(to_hstring(view["move"]["zh"].get<std::string>())));
            Canvas::SetLeft(b, p.first - minx + 28);
            Canvas::SetTop(b, p.second + 14);
            graph.Children().Append(b);
        }
        // Follow the selected variation, then its main continuation.
        std::vector<std::string> route;
        auto id = cursor;
        while (!byid.at(id)["parent"].is_null()) {
            route.push_back(id);
            id = byid.at(id)["parent"].get<std::string>();
        }
        std::reverse(route.begin(), route.end());
        id = cursor;
        while (!byid.at(id)["children"].empty()) {
            id = byid.at(id)["children"][0].get<std::string>();
            route.push_back(id);
        }
        int row = 0;
        Grid moves;
        moves.RowSpacing(2);
        moves.ColumnSpacing(4);
        for (auto width : {34.0, 94.0, 94.0}) {
            ColumnDefinition c;
            c.Width(GridLength{width, GridUnitType::Pixel});
            moves.ColumnDefinitions().Append(c);
        }
        bool previousRed = false;
        for (auto &nodeId : route) {
            auto &n = byid.at(nodeId);
            auto before = byid.at(n["parent"]);
            std::istringstream fen(before["fen"].get<std::string>());
            std::string placement, side, flag1, flag2;
            int half = 0, full = 1;
            fen >> placement >> side >> flag1 >> flag2 >> half >> full;
            bool red = side == "w";
            if (row == 0 || red || !previousRed) {
                RowDefinition r;
                r.Height(GridLength{32, GridUnitType::Pixel});
                moves.RowDefinitions().Append(r);
                TextBlock number;
                number.Text(std::to_wstring(full) + L".");
                number.FontSize(12);
                number.VerticalAlignment(VerticalAlignment::Center);
                number.Foreground(brush(135, 140, 150));
                Grid::SetRow(number, row);
                moves.Children().Append(number);
                ++row;
            }
            auto text = to_hstring(doc().node(std::stoull(nodeId))["move"]["zh"].get<std::string>());
            auto b = button(text.c_str(), [this, nodeId] {
                navigate([this, nodeId] { doc().select(std::stoull(nodeId)); });
            });
            b.Padding(Thickness{4, 0, 4, 0});
            b.Height(30);
            b.MinHeight(0);
            b.MinWidth(0);
            b.HorizontalAlignment(HorizontalAlignment::Stretch);
            b.Background(SolidColorBrush(Color{0, 0, 0, 0}));
            b.BorderThickness(Thickness{0});
            b.FontSize(14);
            if (nodeId == cursor) {
                b.Background(brush(235, 224, 251));
                b.Foreground(brush(109, 40, 217));
            } else
                b.Foreground(red ? brush(153, 35, 35) : brush(39, 49, 65));
            Grid::SetRow(b, row - 1);
            Grid::SetColumn(b, red ? 1 : 2);
            moves.Children().Append(b);
            previousRed = red;
        }
        moveList.Children().Append(moves);
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

    Windows::Foundation::IAsyncAction capture_preview(const std::filesystem::path &path) {
        Microsoft::UI::Xaml::Media::Imaging::RenderTargetBitmap bitmap;
        co_await bitmap.RenderAsync(root);
        auto pixels = co_await bitmap.GetPixelsAsync();
        auto folder =
            co_await Windows::Storage::StorageFolder::GetFolderFromPathAsync(path.parent_path().wstring());
        auto image = co_await folder.CreateFileAsync(
            path.filename().wstring(), Windows::Storage::CreationCollisionOption::ReplaceExisting);
        auto stream = co_await image.OpenAsync(Windows::Storage::FileAccessMode::ReadWrite);
        auto encoder = co_await Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(
            Windows::Graphics::Imaging::BitmapEncoder::PngEncoderId(), stream);
        std::vector<std::uint8_t> data(pixels.Length());
        Windows::Storage::Streams::DataReader::FromBuffer(pixels).ReadBytes(data);
        encoder.SetPixelData(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
                             Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied, bitmap.PixelWidth(),
                             bitmap.PixelHeight(), 96, 96, data);
        co_await encoder.FlushAsync();
        stream.Close();
    }
    void invoke_square(int square) {
        using namespace Microsoft::UI::Xaml::Automation;
        Peers::ButtonAutomationPeer peer(boardView->square(square));
        peer.GetPattern(Peers::PatternInterface::Invoke).as<Provider::IInvokeProvider>().Invoke();
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
            if (boardView->square_count() != 90 || graph.Children().Size() < 7)
                throw std::runtime_error("Native controls did not populate");
            co_await winrt::resume_after(std::chrono::milliseconds(100));
            co_await ui;
            auto stableButton = boardView->square(0);
            auto stableGrid = boardView->grid_image();
            auto stableRook = boardView->piece(0);
            auto movingCannon = boardView->piece(70);
            auto movingParent = movingCannon.Parent();
            auto visualCount = boardView->piece_visual_count();
            auto verifyRetained = [&] {
                if (boardView->square(0) != stableButton || boardView->grid_image() != stableGrid ||
                    boardView->piece(0) != stableRook || boardView->piece_visual_count() != visualCount)
                    throw std::runtime_error("Board visuals were rebuilt during interaction");
            };

            auto beforeClick = doc().current();
            invoke_square(70);
            co_await winrt::resume_after(std::chrono::milliseconds(30));
            co_await ui;
            if (selected != 70)
                throw std::runtime_error("Board source click failed");
            verifyRetained();
            if (boardView->piece(70) != movingCannon)
                throw std::runtime_error("Selection rebuilt its SVG");
            co_await capture_preview(directory / L"selected-preview.png");
            invoke_square(70);
            co_await winrt::resume_after(std::chrono::milliseconds(30));
            co_await ui;
            if (selected != -1)
                throw std::runtime_error("Board selection did not clear");
            verifyRetained();
            co_await capture_preview(directory / L"deselected-preview.png");
            invoke_square(70);
            co_await winrt::resume_after(std::chrono::milliseconds(30));
            co_await ui;
            invoke_square(67);
            co_await winrt::resume_after(std::chrono::milliseconds(30));
            co_await ui;
            if (doc().current() == beforeClick || doc().node(doc().current()).at("move").at("lan") != "h7e7")
                throw std::runtime_error("Board destination click failed");
            verifyRetained();
            if (boardView->piece(67) != movingCannon || boardView->piece(70) ||
                movingCannon.Parent() != movingParent)
                throw std::runtime_error("Moved SVG visual was not reused");
            if (boardView->last_move_visibility() != Visibility::Visible)
                throw std::runtime_error("Last-move marker missing after move");
            for (int repeat = 0; repeat < 2; ++repeat) {
                boardView->square(67).Focus(FocusState::Pointer);
                invoke_square(67);
                co_await winrt::resume_after(std::chrono::milliseconds(30));
                co_await ui;
                auto border = boardView->square(67).BorderThickness();
                if (selected != 67 || boardView->last_move_visibility() != Visibility::Collapsed ||
                    border.Left || border.Top || border.Right || border.Bottom ||
                    boardView->square(67).Shadow())
                    throw std::runtime_error("Recently moved selection retained a decoration");
                verifyRetained();
                if (boardView->piece(67) != movingCannon || movingCannon.Parent() != movingParent)
                    throw std::runtime_error("Recently moved selection rebuilt its piece");
                if (repeat == 0)
                    co_await capture_preview(directory / L"recent-move-selected-preview.png");
                invoke_square(67);
                co_await winrt::resume_after(std::chrono::milliseconds(30));
                co_await ui;
                if (selected != -1 || boardView->last_move_visibility() != Visibility::Visible)
                    throw std::runtime_error("Last-move marker not restored after deselection");
            }
            doc().select(beforeClick);
            refresh();
            auto capturingCannon = boardView->piece(64);
            auto capturedHorse = boardView->piece(1);
            navigate([this] { doc().play(64, 1); });
            if (boardView->piece(1) != capturingCannon || boardView->piece(64))
                throw std::runtime_error("Capture did not reuse its SVG visual");
            navigate([this] {
                if (!doc().undo())
                    throw std::runtime_error("Capture undo failed");
            });
            if (boardView->piece(1) != capturedHorse || boardView->piece(64) != capturingCannon)
                throw std::runtime_error("Captured SVG was not retained for undo");
            verifyRetained();

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
            if (!failedPieceArt.empty() || loadedPieceArt.size() != 14)
                throw std::runtime_error("OXDF SVG resources failed to decode");
            co_await capture_preview(directory / L"native-preview.png");
            bamboo = true; // Internal rendering check; no public style selector.
            flipped = true;
            draw_board();
            refresh_evaluation_strip();
            co_await winrt::resume_after(std::chrono::milliseconds(300));
            co_await ui;
            co_await capture_preview(directory / L"internal-bamboo-preview.png");
            flipped = false;
            bamboo = false;
            refresh();

            xqn::save_atomic(directory / L"result.json",
                             bytes(xqn::Json{{"passed", true},
                                             {"boardControls", boardView->square_count()},
                                             {"boardLayers", boardView->layer_count()},
                                             {"retainedBoard", true},
                                             {"pieceVisuals", visualCount},
                                             {"graphControls", graph.Children().Size()},
                                             {"gameNodes", doc().model().move_tree.nodes.size()},
                                             {"preview", "native-preview.png"},
                                             {"boardClick", true},
                                             {"pieceResources", loadedPieceArt.size()},
                                             {"visibleBoardStyles", 1},
                                             {"internalBoardStylesTested", 2},
                                             {"recentMoveSelection", true},
                                             {"selectionScale", 1.10}}
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
