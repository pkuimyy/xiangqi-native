# Xiangqi Native

独立 Windows 原生象棋应用，使用 **WinUI 3、C++/WinRT、C++20 与 OXQF**。棋规、中文记谱、PGN 和分支布局已迁为 C++；源码构建与应用运行均无需 Node.js、JavaScript 运行时或 Obsidian。

![原生应用预览](docs/assets/native-preview.png)

目前为持续开发中的可运行版本，尚未覆盖原插件全部功能。实际进度见 [docs/PROGRESS.md](docs/PROGRESS.md)。

## 运行

本机可运行程序：`build/winui/Release/XiangqiNative.exe`。复制应用时保留整个 Release 目录，Windows App SDK 运行库和素材随目录部署。当前验证平台为 Windows 11 x64。

- 原木纹、竹纹棋盘和传统棋子资源；WinUI 原生按钮、菜单式选择器、文件对话框和文本框。
- 点击棋子与落点走子；支持棋盘翻转、合法落点、上一着高亮和 FEN 复制。
- 紧凑分支树与双列中文着法，节点点击跳转、提升主线、删除分支、撤销/重做。
- 中文、WXF、ICCS 输入；PGN 多局导入、评语与分析信息保留；单局 OXQ 保存及 PGN 导出。
- 原生皮卡鱼进程分析、停止分析、最佳着箭头、红方视角分数和评估条。

可选引擎安装：

```powershell
./scripts/setup-engine.ps1 -Configuration Release
```

程序自动发现 engines 目录，也可在界面选择已有引擎。

## 构建与验证

需要 Visual Studio C++ 桌面工具、Windows SDK 10.0.26100.0、CMake/Ninja，以及首次还原 WinUI NuGet 包的网络连接。支持 VS 2022 v143 与当前开发机 VS v145；使用锁定的 Windows App SDK 和 C++/WinRT 包。无需安装 npm。

```powershell
./scripts/build.ps1 -Configuration Release
./scripts/smoke.ps1 -Configuration Release
./scripts/build.ps1 -Configuration Debug -CoreOnly
```

构建会运行 C++ 文档、引擎和旧实现对照测试。自测会启动实际 WinUI 应用，验证按钮走子、OXQ 保存往返、木纹/竹纹切换及翻转渲染，输出位于 `build/winui/Release/smoke/`。

旧实现对照包含 600 个对局局面、23,063 个合法走法、8 个残局/重子局面、25 组 PGN 输入及 232 节点复杂分支布局。它们是迁移前保存的 JSON 数据，执行测试不需要旧 JS 实现。

## 源码

业务核心在 src/core，原生界面在 app，OXQF 源码固定于 external/oxqf。详见 [架构](docs/ARCHITECTURE.md)、[依赖与素材来源](docs/DEPENDENCIES.md)。

项目保留原应用 GPL-3.0 许可及第三方授权说明。原 Obsidian 项目和 OXQF 工作目录未被修改。
