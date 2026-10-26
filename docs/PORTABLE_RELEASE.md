# Portable release packaging

The normal-user distribution is a ZIP folder, without an installer. Build output remains build/winui/Release/XiangqiNative.exe. Package artifacts go to build/packages.

## Create a release

From the repository root:

    .\scripts\package.ps1

This builds Release with full regression tests skipped, then packages and verifies it. To package an already up-to-date build:

    .\scripts\package.ps1 -SkipBuild

Optional:

    .\scripts\package.ps1 -Version 0.1.0
    .\scripts\package.ps1 -SkipBuild -WithoutEngine
    .\scripts\package.ps1 -SkipBuild -Languages '*'

The default package includes Pikafish and NNUE. If these are absent, run scripts/setup-engine.ps1 -Configuration Release first. A -WithoutEngine package has a separate no-engine filename. Default SDK satellite resources are en-US, zh-CN and zh-TW; '*' retains every supplied SDK locale. The application's own labels remain Chinese.

New source files under app/src/include/assets/external/scripts/packaging/cmake must be staged or ignored before packaging. The source archive collects current contents of tracked files, including tracked modifications. -SkipBuild should only be used when the executable matches those contents.

## Outputs

- XiangqiNative-win-x64-portable.zip: the user application folder, with app-local runtime, active SVG assets, engine/network, notices and an offline HTML user guide.
- XiangqiNative-source.zip: separate corresponding source, the actual fetched OXQF snapshot, and the official pinned Pikafish source archive when the engine is included.
- XiangqiNative-SHA256SUMS.txt: checksums for the two archives.
- XiangqiNative-win-x64-portable.files.json: maintainer file inventory, hashes and verification results.

Version and no-engine options change the output basenames. Publish the portable ZIP and its matching source/checksum files together. The inventory is useful for maintainers and is outside the user ZIP.

PDB/ILK/OBJ/LIB files, tests, screenshots, logs, developer scripts, personal Data/settings, saved chess documents, generation-only SVG inputs and hidden bamboo artwork are excluded from the user ZIP. WinUI runtime DLL/PRI/WINMD files are retained as deployment components; SDK metadata is not a debug-symbol file. Duplicate WebView loader subtrees and the SDK designer DLL are excluded. The runtime dependency audit checks normal and delayed x64 PE imports and collects VC CRT redistributables only if imported; OS API-set contracts remain Windows dependencies.

## Portable settings and updates

portable.flag makes the app use Data/settings.json beside the executable. The package starts without Data and never copies the developer's settings. Paths for engines inside the application folder are saved relatively and resolved from the executable location, independent of the working directory and Windows path casing.

The regular developer output, without portable.flag, retains its existing LOCALAPPDATA settings location. Adding the marker enables portable behavior there as well.

Users should extract to a writable folder. Upgrade by closing the old app, extracting the new version to a new folder, and copying Data from the old folder. Chess documents saved elsewhere should be copied separately. External engine paths remain external; moving them into the app folder makes them portable too.

## Verification

The script creates a ZIP, extracts it into an isolated directory with Chinese characters and spaces, and starts it from an unrelated working directory. The app self-check verifies app-local WinUI runtime modules, 14 SVG resources, portable preference storage, relative engine paths and bundled live analysis. It then copies only the first instance's Data/settings.json to a second extraction and verifies restoration while the first extraction still exists. This catches lingering references to the old folder.

Verification output and generated Data are confined to temporary extraction directories. Those directories are path-checked before recursive cleanup. The ZIP itself never receives test output or user configuration. Final ZIPs are published only after verification succeeds.

Use -SkipVerification only for a build service without an interactive Windows desktop. The CI artifact workflow uses that option; perform interactive verification before publishing to users. This is not a substitute for a clean Windows 11 VM test. The package has been verified on the current development machine, with app-local runtime checks and PE import auditing.

PowerShell 5.1 and PowerShell 7 are supported. ZIP entry names use explicit UTF-8 and standard forward slashes on both runtimes. End users need neither PowerShell nor a compiler to run the extracted app. The source companion can build with the local unmodified OXQF snapshot using scripts/build.ps1 -OxqfSourceDirectory; see its SOURCE-BUILD.txt.

## Deployment references

Microsoft documents xcopy deployment for unpackaged, self-contained Windows App SDK apps:
https://learn.microsoft.com/windows/apps/package-and-deploy/self-contained-deploy/deploy-self-contained-apps

VC app-local deployment, when needed:
https://learn.microsoft.com/cpp/windows/choosing-a-deployment-method

Pikafish's pinned source and original license/NNUE conditions are included or linked in the user notices and source archive. No source or binary is published to a remote service by the packaging script.
