#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <vector>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>

namespace xqn::ui {
namespace sx = winrt::Microsoft::UI::Xaml;
namespace sc = sx::Controls;
namespace sm = sx::Media;
enum class ShellCommand {
    NewGame,
    Open,
    Save,
    Export,
    Undo,
    Redo,
    Flip,
    Promote,
    Erase,
    First,
    Back,
    Next,
    Last,
    Play,
    Comment,
    Engine,
    Analyze,
    Stop,
    CopyFen
};
struct WorkspaceWidgets {
    sc::Canvas board, graph;
    sc::StackPanel moves;
    sc::Grid turnStrip;
    sc::Border turnIndicator;
    sc::TextBlock turn, evaluation, status, analysisState, variation;
    sc::TextBox comment, notation;
    sc::ToggleSwitch analysis;
};

// A provisional native shell; document operations remain owned by Workspace.
class WorkspaceLayout {
  public:
    WorkspaceLayout(sc::Grid root, WorkspaceWidgets widgets, double boardWidth,
                    std::function<void(ShellCommand)> command)
        : root_(root), w_(widgets), command_(std::move(command)), boardWidth_(boardWidth) {
        root_.Background(ink(243, 243, 243));
        root_.Padding(sx::Thickness{12, 8, 12, 8});
        root_.RowSpacing(8);
        for (auto unit : {sx::GridUnitType::Auto, sx::GridUnitType::Auto, sx::GridUnitType::Star,
                          sx::GridUnitType::Auto}) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, unit});
            root_.RowDefinitions().Append(row);
        }
        create_commands();
        create_board();
        create_study();
        content_.ColumnSpacing(12);
        content_.Height(boardWidth_ * 548.0 / 466.0 + 72);
        content_.HorizontalAlignment(sx::HorizontalAlignment::Left);
        content_.VerticalAlignment(sx::VerticalAlignment::Top);
        for (auto unit : {sx::GridUnitType::Pixel, sx::GridUnitType::Pixel, sx::GridUnitType::Pixel}) {
            sc::ColumnDefinition column;
            column.Width(sx::GridLength{1, unit});
            content_.ColumnDefinitions().Append(column);
        }
        content_.ColumnDefinitions().GetAt(0).Width(sx::GridLength{boardWidth_, sx::GridUnitType::Pixel});
        content_.ColumnDefinitions().GetAt(1).Width(sx::GridLength{252, sx::GridUnitType::Pixel});
        content_.ColumnDefinitions().GetAt(2).Width(sx::GridLength{0, sx::GridUnitType::Pixel});
        content_.Children().Append(boardPane_);
        sc::Grid::SetColumn(sidebar_, 1);
        content_.Children().Append(sidebar_);
        sc::Grid::SetColumn(wideTree_, 2);
        content_.Children().Append(wideTree_);

        workspaceScroll_.Content(content_);
        workspaceScroll_.HorizontalContentAlignment(sx::HorizontalAlignment::Left);
        workspaceScroll_.VerticalContentAlignment(sx::VerticalAlignment::Top);
        workspaceScroll_.HorizontalScrollBarVisibility(sc::ScrollBarVisibility::Auto);
        workspaceScroll_.VerticalScrollBarVisibility(sc::ScrollBarVisibility::Auto);
        sc::Grid::SetRow(workspaceScroll_, 2);
        root_.Children().Append(workspaceScroll_);
        w_.status.FontSize(11);
        w_.status.Foreground(ink(105, 110, 120));
        w_.status.TextTrimming(sx::TextTrimming::CharacterEllipsis);
        w_.status.MaxLines(1);
        sc::Grid::SetRow(w_.status, 3);
        root_.Children().Append(w_.status);
        root_.SizeChanged([this](auto &&, auto &&) { adapt(); });
        select_ribbon(0);
        select_study(false);
    }

    void show_analysis() { select_ribbon(1); }

    void fit_content(double moves, double tree) {
        moveExtent_ = moves;
        treeExtent_ = tree;
        adapt();
    }
    void adapt() {
        double width = root_.ActualWidth();
        if (width <= 0)
            return;
        bool wide = width >= 1120;
        double sidebarWidth = wide ? (std::clamp)((width - boardWidth_) * .30, 260.0, 320.0)
                                   : (std::clamp)(width - boardWidth_ - 48.0, 228.0, 320.0);
        double researchWidth =
            wide ? (std::clamp)(width - boardWidth_ - sidebarWidth - 48.0, 300.0, 600.0) : 0;
        double desiredWidth = boardWidth_ + sidebarWidth + researchWidth + 24;
        double bodyHeight =
            (std::max)(boardWidth_ * 548.0 / 466.0 + 72, (std::min)(900.0, root_.ActualHeight() - 130));
        content_.Width(desiredWidth);
        content_.Height(bodyHeight);
        content_.ColumnDefinitions().GetAt(1).Width(sx::GridLength{sidebarWidth, sx::GridUnitType::Pixel});
        content_.ColumnDefinitions().GetAt(2).Width(sx::GridLength{researchWidth, sx::GridUnitType::Pixel});
        double gutter = (std::max)(0.0, (width - 24 - desiredWidth) / 2);
        content_.HorizontalAlignment(sx::HorizontalAlignment::Left);
        content_.Margin(sx::Thickness{gutter, 0, gutter, 0});
        workspaceScroll_.HorizontalContentAlignment(sx::HorizontalAlignment::Left);
        boardPane_.VerticalAlignment(wide ? sx::VerticalAlignment::Center : sx::VerticalAlignment::Top);
        if (wide != wide_) {
            if (wide) {
                move_card(treeCard_, studyViews_, wideTreeBody_, 0);
                move_card(analysisCard_, sidebar_, wideTreeBody_, 1);
                move_card(notesCard_, sidebar_, wideTreeBody_, 2);
            } else {
                move_card(treeCard_, wideTreeBody_, studyViews_, 0);
                move_card(analysisCard_, wideTreeBody_, sidebar_, 3);
                move_card(notesCard_, wideTreeBody_, sidebar_, 4);
            }
            wide_ = wide;
        }
        double noteHeight = wide ? (std::clamp)(bodyHeight * .30, 200.0, 300.0) : 176;
        double analysisHeight = 144;
        sidebar_.Height(wide ? boardWidth_ * 548.0 / 466.0 + 72 : bodyHeight);
        sidebar_.VerticalAlignment(wide ? sx::VerticalAlignment::Center : sx::VerticalAlignment::Top);
        notesCard_.Height(noteHeight);
        w_.comment.Height(noteHeight - 92);
        analysisCard_.Height(analysisHeight);
        w_.variation.MaxLines(wide ? 3 : 2);
        double studyHeight = wide ? bodyHeight - 40 : bodyHeight - noteHeight - analysisHeight - 96;
        moveCard_.MaxHeight((std::max)(80.0, (std::min)(moveExtent_, studyHeight)));
        treeCard_.Height((std::max)(
            120.0,
            (std::min)(treeExtent_, wide ? bodyHeight - noteHeight - analysisHeight - 56 : studyHeight)));
        wideTree_.Visibility(wide ? sx::Visibility::Visible : sx::Visibility::Collapsed);
        studySwitch_.Visibility(wide ? sx::Visibility::Collapsed : sx::Visibility::Visible);
        select_study(treeSelected_);
    }

  private:
    sc::Grid root_, content_, boardPane_, sidebar_, studyViews_, wideTree_, wideTreeBody_;
    sc::Border treeCard_, moveCard_, analysisCard_, notesCard_;
    sc::ScrollViewer workspaceScroll_;
    sc::StackPanel studySwitch_;
    WorkspaceWidgets w_;
    std::function<void(ShellCommand)> command_;
    std::array<sc::CommandBar, 3> ribbons_;
    std::vector<sc::Primitives::ToggleButton> tabs_, studyTabs_;
    double boardWidth_;
    double moveExtent_ = 100, treeExtent_ = 180;
    bool wide_ = false, treeSelected_ = false;
    static void move_card(sc::Border card, sc::Grid from, sc::Grid to, int row) {
        uint32_t index;
        if (from.Children().IndexOf(card, index))
            from.Children().RemoveAt(index);
        sc::Grid::SetRow(card, row);
        to.Children().Append(card);
    }
    static sm::SolidColorBrush ink(unsigned r, unsigned g, unsigned b) {
        return sm::SolidColorBrush(winrt::Windows::UI::Color{
            255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    }
    static sc::TextBlock text(wchar_t const *value, double size = 14) {
        sc::TextBlock result;
        result.Text(value);
        result.FontSize(size);
        result.VerticalAlignment(sx::VerticalAlignment::Center);
        return result;
    }
    sc::Button button(wchar_t const *label, ShellCommand command) {
        sc::Button result;
        result.Content(winrt::box_value(label));
        result.Padding(sx::Thickness{10, 4, 10, 4});
        result.Click([this, command](auto &&, auto &&) { command_(command); });
        return result;
    }
    sc::AppBarButton action(wchar_t const *label, sc::Symbol icon, ShellCommand command) {
        sc::AppBarButton result;
        result.Label(label);
        result.Icon(sc::SymbolIcon(icon));
        result.Click([this, command](auto &&, auto &&) { command_(command); });
        return result;
    }
    void select_ribbon(int selected) {
        for (int i = 0; i < 3; ++i) {
            tabs_[i].IsChecked(i == selected);
            ribbons_[i].Visibility(i == selected ? sx::Visibility::Visible : sx::Visibility::Collapsed);
        }
    }
    void select_study(bool tree) {
        treeSelected_ = tree;
        for (int i = 0; i < 2; ++i)
            studyTabs_[i].IsChecked(tree == (i == 1));
        moveCard_.Visibility(wide_ || !tree ? sx::Visibility::Visible : sx::Visibility::Collapsed);
        treeCard_.Visibility(wide_ || tree ? sx::Visibility::Visible : sx::Visibility::Collapsed);
    }
    void create_commands() {
        sc::Grid header;
        sc::ColumnDefinition a, b;
        a.Width(sx::GridLength{1, sx::GridUnitType::Star});
        b.Width(sx::GridLength{1, sx::GridUnitType::Auto});
        header.ColumnDefinitions().Append(a);
        header.ColumnDefinitions().Append(b);
        sc::StackPanel menu;
        menu.Orientation(sc::Orientation::Horizontal);
        menu.Spacing(4);
        sc::DropDownButton file;
        file.Content(winrt::box_value(L"文件"));
        file.Padding(sx::Thickness{10, 4, 10, 4});
        sc::MenuFlyout flyout;
        for (auto entry : {std::pair{L"新建棋谱", ShellCommand::NewGame},
                           {L"打开…", ShellCommand::Open},
                           {L"保存", ShellCommand::Save},
                           {L"导出 PGN…", ShellCommand::Export}}) {
            sc::MenuFlyoutItem item;
            item.Text(entry.first);
            item.Click([this, command = entry.second](auto &&, auto &&) { command_(command); });
            flyout.Items().Append(item);
        }
        file.Flyout(flyout);
        menu.Children().Append(file);
        for (auto label : {L"开始", L"分析", L"视图"}) {
            sc::Primitives::ToggleButton tab;
            tab.Content(winrt::box_value(label));
            tab.Padding(sx::Thickness{12, 4, 12, 4});
            int index = static_cast<int>(tabs_.size());
            tab.Click([this, index](auto &&, auto &&) { select_ribbon(index); });
            tabs_.push_back(tab);
            menu.Children().Append(tab);
        }
        header.Children().Append(menu);
        auto hint = text(L"棋谱工作区", 12);
        hint.Foreground(ink(110, 115, 125));
        sc::Grid::SetColumn(hint, 1);
        header.Children().Append(hint);
        root_.Children().Append(header);
        sc::Border ribbonCard;
        ribbonCard.Background(ink(250, 250, 250));
        ribbonCard.CornerRadius(sx::CornerRadius{6});
        sc::Grid ribbonHost;
        for (auto ribbon : ribbons_) {
            ribbon.Background(ink(250, 250, 250));
            ribbon.HorizontalAlignment(sx::HorizontalAlignment::Left);
            ribbon.DefaultLabelPosition(sc::CommandBarDefaultLabelPosition::Right);
            ribbon.IsDynamicOverflowEnabled(true);
            ribbonHost.Children().Append(ribbon);
        }
        auto add = [&](int index, wchar_t const *label, sc::Symbol icon, ShellCommand command) {
            ribbons_[index].PrimaryCommands().Append(action(label, icon, command));
        };
        add(0, L"保存", sc::Symbol::Save, ShellCommand::Save);
        add(0, L"撤销", sc::Symbol::Undo, ShellCommand::Undo);
        add(0, L"重做", sc::Symbol::Redo, ShellCommand::Redo);
        ribbons_[0].PrimaryCommands().Append(sc::AppBarSeparator());
        add(0, L"提升主线", sc::Symbol::Up, ShellCommand::Promote);
        add(0, L"删除分支", sc::Symbol::Delete, ShellCommand::Erase);
        add(1, L"选择引擎", sc::Symbol::Setting, ShellCommand::Engine);
        sc::AppBarElementContainer explanation;
        auto message = text(L"常驻局面分析 · 单线程 · 后台运行", 12);
        message.Margin(sx::Thickness{12, 0, 12, 0});
        explanation.Height(48);
        explanation.VerticalContentAlignment(sx::VerticalAlignment::Center);
        explanation.Content(message);
        ribbons_[1].PrimaryCommands().Append(explanation);
        add(2, L"翻转棋盘", sc::Symbol::Refresh, ShellCommand::Flip);
        add(2, L"复制 FEN", sc::Symbol::Copy, ShellCommand::CopyFen);
        ribbonCard.Height(48);
        ribbonCard.Child(ribbonHost);
        sc::Grid::SetRow(ribbonCard, 1);
        root_.Children().Append(ribbonCard);
    }
    void create_board() {
        boardPane_.Width(boardWidth_);
        boardPane_.RowSpacing(8);
        boardPane_.VerticalAlignment(sx::VerticalAlignment::Top);
        for (int i = 0; i < 3; ++i) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, sx::GridUnitType::Auto});
            boardPane_.RowDefinitions().Append(row);
        }
        w_.turn.FontSize(14);
        w_.turn.Height(24);
        w_.turn.Margin(sx::Thickness{16, 0, 0, 0});
        boardPane_.Children().Append(w_.turn);
        w_.board.Width(450);
        w_.board.Height(548);
        sc::Grid surface;
        surface.Width(466);
        surface.Height(548);
        surface.ColumnSpacing(8);
        for (double width : {8.0, 450.0}) {
            sc::ColumnDefinition col;
            col.Width(sx::GridLength{width, sx::GridUnitType::Pixel});
            surface.ColumnDefinitions().Append(col);
        }
        for (int i = 0; i < 2; ++i) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, sx::GridUnitType::Star});
            w_.turnStrip.RowDefinitions().Append(row);
        }
        w_.turnIndicator.CornerRadius(sx::CornerRadius{4});
        w_.turnStrip.Children().Append(w_.turnIndicator);
        sc::Border strip;
        strip.Background(ink(236, 239, 243));
        strip.BorderBrush(ink(206, 210, 218));
        strip.BorderThickness(sx::Thickness{1});
        strip.CornerRadius(sx::CornerRadius{4});
        strip.Child(w_.turnStrip);
        surface.Children().Append(strip);
        sc::Grid::SetColumn(w_.board, 1);
        surface.Children().Append(w_.board);
        sc::Viewbox view;
        view.Stretch(sm::Stretch::Uniform);
        view.Width(boardWidth_);
        view.Height(boardWidth_ * 548.0 / 466.0);
        view.Child(surface);
        sc::Grid::SetRow(view, 1);
        boardPane_.Children().Append(view);

        sc::Grid controls;
        controls.Height(32);
        controls.ColumnSpacing(6);
        for (auto unit : {sx::GridUnitType::Auto, sx::GridUnitType::Star, sx::GridUnitType::Auto}) {
            sc::ColumnDefinition column;
            column.Width(sx::GridLength{1, unit});
            controls.ColumnDefinitions().Append(column);
        }
        sc::StackPanel navigation;
        navigation.Orientation(sc::Orientation::Horizontal);
        navigation.Spacing(2);
        for (auto entry : {std::pair{L"|◀", ShellCommand::First},
                           {L"◀", ShellCommand::Back},
                           {L"▶", ShellCommand::Next},
                           {L"▶|", ShellCommand::Last},
                           {L"↻", ShellCommand::Flip}}) {
            auto control = button(entry.first, entry.second);
            control.Width(26);
            control.Height(32);
            control.MinWidth(0);
            control.MinHeight(0);
            control.Padding(sx::Thickness{0});
            control.FontSize(12);
            if (entry.second == ShellCommand::Flip)
                control.Content(sc::SymbolIcon(sc::Symbol::Refresh));
            sc::ToolTipService::SetToolTip(
                control, winrt::box_value(entry.second == ShellCommand::First  ? L"回到开始"
                                          : entry.second == ShellCommand::Back ? L"上一着（←）"
                                          : entry.second == ShellCommand::Next ? L"下一着（→）"
                                          : entry.second == ShellCommand::Last ? L"到末着"
                                                                               : L"翻转棋盘"));
            navigation.Children().Append(control);
        }
        controls.Children().Append(navigation);
        w_.notation.MinWidth(0);
        w_.notation.Height(32);
        w_.notation.FontSize(13);
        w_.notation.Padding(sx::Thickness{6, 4, 6, 4});
        w_.notation.PlaceholderText(L"输入着法");
        sc::ToolTipService::SetToolTip(w_.notation, winrt::box_value(L"如 炮二平五 / H2-E2"));
        sc::Grid::SetColumn(w_.notation, 1);
        controls.Children().Append(w_.notation);
        auto play = button(L"走子", ShellCommand::Play);
        play.Width(40);
        play.Height(32);
        play.MinWidth(0);
        play.Padding(sx::Thickness{4, 0, 4, 0});
        sc::Grid::SetColumn(play, 2);
        controls.Children().Append(play);
        sc::Grid::SetRow(controls, 2);
        boardPane_.Children().Append(controls);
    }
    void create_study() {
        sidebar_.RowSpacing(8);
        for (auto unit : {sx::GridUnitType::Auto, sx::GridUnitType::Auto, sx::GridUnitType::Star,
                          sx::GridUnitType::Auto, sx::GridUnitType::Auto}) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, unit});
            sidebar_.RowDefinitions().Append(row);
        }
        auto name = text(L"棋谱", 16);
        name.Height(32);
        sidebar_.Children().Append(name);
        studySwitch_.Orientation(sc::Orientation::Horizontal);
        studySwitch_.Spacing(4);
        for (auto name : {L"着法", L"图形树"}) {
            sc::Primitives::ToggleButton tab;
            tab.Content(winrt::box_value(name));
            tab.Padding(sx::Thickness{12, 4, 12, 4});
            bool tree = !studyTabs_.empty();
            tab.Click([this, tree](auto &&, auto &&) { select_study(tree); });
            studyTabs_.push_back(tab);
            studySwitch_.Children().Append(tab);
        }
        sc::Grid::SetRow(studySwitch_, 1);
        sidebar_.Children().Append(studySwitch_);
        sc::ScrollViewer moveScroll, graphScroll;
        moveScroll.Content(w_.moves);
        moveScroll.HorizontalContentAlignment(sx::HorizontalAlignment::Stretch);
        moveScroll.VerticalScrollBarVisibility(sc::ScrollBarVisibility::Auto);
        moveScroll.HorizontalScrollBarVisibility(sc::ScrollBarVisibility::Disabled);
        w_.graph.VerticalAlignment(sx::VerticalAlignment::Top);
        w_.graph.HorizontalAlignment(sx::HorizontalAlignment::Left);
        graphScroll.Content(w_.graph);
        graphScroll.HorizontalScrollBarVisibility(sc::ScrollBarVisibility::Auto);
        graphScroll.VerticalScrollBarVisibility(sc::ScrollBarVisibility::Auto);
        graphScroll.HorizontalContentAlignment(sx::HorizontalAlignment::Left);
        graphScroll.VerticalContentAlignment(sx::VerticalAlignment::Top);
        moveCard_.Child(moveScroll);
        treeCard_.Child(graphScroll);
        for (auto card : {moveCard_, treeCard_}) {
            card.Background(ink(255, 255, 255));
            card.BorderBrush(ink(226, 228, 232));
            card.BorderThickness(sx::Thickness{1});
            card.CornerRadius(sx::CornerRadius{6});
            card.VerticalAlignment(sx::VerticalAlignment::Top);
            studyViews_.Children().Append(card);
        }
        sc::Grid::SetRow(studyViews_, 2);
        sidebar_.Children().Append(studyViews_);

        analysisCard_.Background(ink(250, 250, 250));
        analysisCard_.BorderBrush(ink(226, 228, 232));
        analysisCard_.BorderThickness(sx::Thickness{1});
        analysisCard_.CornerRadius(sx::CornerRadius{6});
        analysisCard_.Padding(sx::Thickness{10});
        analysisCard_.Height(144);
        sc::Grid analysis;
        analysis.RowSpacing(4);
        for (auto height :
             {sx::GridLength{28, sx::GridUnitType::Pixel}, sx::GridLength{16, sx::GridUnitType::Pixel},
              sx::GridLength{22, sx::GridUnitType::Pixel}, sx::GridLength{1, sx::GridUnitType::Star}}) {
            sc::RowDefinition row;
            row.Height(height);
            analysis.RowDefinitions().Append(row);
        }
        sc::Grid heading;
        sc::ColumnDefinition label, control;
        label.Width(sx::GridLength{1, sx::GridUnitType::Star});
        control.Width(sx::GridLength{1, sx::GridUnitType::Auto});
        heading.ColumnDefinitions().Append(label);
        heading.ColumnDefinitions().Append(control);
        heading.Children().Append(text(L"局面分析", 13));
        w_.analysis.MinWidth(0);
        w_.analysis.Width(48);
        w_.analysis.OnContent(winrt::box_value(L""));
        w_.analysis.OffContent(winrt::box_value(L""));
        sc::ToolTipService::SetToolTip(w_.analysis, winrt::box_value(L"自动分析当前局面"));
        sc::Grid::SetColumn(w_.analysis, 1);
        heading.Children().Append(w_.analysis);
        analysis.Children().Append(heading);
        w_.analysisState.FontSize(11);
        w_.analysisState.Foreground(ink(100, 108, 120));
        w_.analysisState.TextTrimming(sx::TextTrimming::CharacterEllipsis);
        sc::Grid::SetRow(w_.analysisState, 1);
        analysis.Children().Append(w_.analysisState);
        w_.evaluation.FontSize(15);
        w_.evaluation.TextTrimming(sx::TextTrimming::CharacterEllipsis);
        sc::Grid::SetRow(w_.evaluation, 2);
        analysis.Children().Append(w_.evaluation);
        w_.variation.FontSize(12);
        w_.variation.Foreground(ink(85, 94, 108));
        w_.variation.TextWrapping(sx::TextWrapping::Wrap);
        w_.variation.MaxLines(2);
        sc::Grid::SetRow(w_.variation, 3);
        analysis.Children().Append(w_.variation);
        analysisCard_.Child(analysis);
        sc::Grid::SetRow(analysisCard_, 3);
        sidebar_.Children().Append(analysisCard_);

        notesCard_.Background(ink(250, 250, 250));
        notesCard_.CornerRadius(sx::CornerRadius{6});
        notesCard_.BorderBrush(ink(226, 228, 232));
        notesCard_.BorderThickness(sx::Thickness{1});
        notesCard_.Padding(sx::Thickness{12});
        notesCard_.Height(176);
        sc::StackPanel notesBody;
        notesBody.Spacing(6);
        notesBody.Children().Append(text(L"当前着注释", 13));
        w_.comment.AcceptsReturn(true);
        w_.comment.TextWrapping(sx::TextWrapping::Wrap);
        w_.comment.Height(80);
        w_.comment.PlaceholderText(L"记录当前局面的想法…");
        notesBody.Children().Append(w_.comment);
        auto save = button(L"保存注释", ShellCommand::Comment);
        save.HorizontalAlignment(sx::HorizontalAlignment::Right);
        notesBody.Children().Append(save);
        notesCard_.Child(notesBody);
        sc::Grid::SetRow(notesCard_, 4);
        sidebar_.Children().Append(notesCard_);

        wideTree_.RowSpacing(8);
        wideTree_.VerticalAlignment(sx::VerticalAlignment::Center);
        for (int i = 0; i < 2; ++i) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, sx::GridUnitType::Auto});
            wideTree_.RowDefinitions().Append(row);
        }
        auto treeTitle = text(L"研究区", 16);
        treeTitle.Height(32);
        wideTree_.Children().Append(treeTitle);
        wideTreeBody_.RowSpacing(8);
        for (int i = 0; i < 3; ++i) {
            sc::RowDefinition row;
            row.Height(sx::GridLength{1, sx::GridUnitType::Auto});
            wideTreeBody_.RowDefinitions().Append(row);
        }
        sc::Grid::SetRow(wideTreeBody_, 1);
        wideTree_.Children().Append(wideTreeBody_);
        wideTree_.Visibility(sx::Visibility::Collapsed);
    }
};
} // namespace xqn::ui
