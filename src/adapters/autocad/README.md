# AutoCAD adapter — Duct System Designer

This folder is the AutoCAD adapter within the Duct System Designer checkout.
Its ARX target compiles the adapter and application sources and links the core
static library. Keep the sibling `src/core` and `src/application` folders available
when using the adapter as the CMake entry point.

## Structure

```text
autocad/
  CMakeLists.txt
  README.md
  include/autocad/
    commands/UiCommands.h           Command registration API
    ui/
      Designer.h                   Palette show/hide/select API
      PalettePage.h                Shared native palette bridge
      controls/                    Styled native control classes
      layout/                      Scrollable forms and horizontal tabs
      model/                       UI preview/session state
      panels/                      Panel classes, control IDs and handlers
      platform/                    MFC / ObjectARX include boundaries
      style/                       Colors, fonts and drawing helpers
    document/                      Reserved for document integration
    database/                      Reserved for drawing database integration
  src/
    Module.cpp                     Sole DLL/ObjectARX entry point
    commands/UiCommands.cpp         Existing AutoCAD command registrations
    ui/
      Designer.cpp                 Palette ownership, docking and main tab order
      PalettePage.cpp              Native palette sizing, focus and activation
      controls/                    Control message maps
      layout/                      Layout message maps
      model/                       Session state definitions
      panels/                      Panel message maps and factories
    document/                      Reserved
    database/                      Reserved
  assets/fonts/                    Inter fonts and license
  cmake/FindObjectARX.cmake         Standalone fallback SDK finder
  cmake/CheckConfiguration.cmake   Reject unsupported Debug host builds
  tests/                           Native MFC regression harness
  build.cmd                        CMake wrapper
  build-msvc.cmd                   Direct cl/link fallback
```

Headers use project-qualified includes such as `autocad/ui/Designer.h` and
`autocad/ui/controls/UiButton.h`; the compiler include root is `autocad/include`.
File names and the existing DuctUi namespace are retained. The model directory
holds MFC UI/session state; the plain C++ project-information model and registration
service live under `src/application`.
The document/database folders contain placeholders only.

## Build from the project root

The root CMake already adds core, application source registration, and the adapter
when `DSD_BUILD_AUTOCAD` is enabled. This option defaults to `ON` on Windows and
`OFF` elsewhere.

The adapter uses its bundled ObjectARX finder. A parent project can supply its own
finder earlier in `CMAKE_MODULE_PATH`, or an existing imported target
`ObjectARX::ObjectARX` carrying SDK include directories and libraries, and
`OBJECTARX_DEFINITION_FILE` pointing to `inc/AcRxDefault.def`. Alternatively set
`OBJECTARX_ROOT` so the adapter can derive that definition-file path. You may
move the bundled finder to your root `cmake` directory as your central SDK finder.

Build from the completed project root:

```bat
cmake -S . -B build-arx -G "Visual Studio 17 2022" -A x64 -DDSD_BUILD_AUTOCAD=ON -DOBJECTARX_ROOT="C:/path/to/ObjectARX2027" -DBUILD_TESTING=OFF
cmake --build build-arx --config Release --target DuctSystemDesignerUi
```

This creates the ARX application target. `src/application/CMakeLists.txt` attaches
application sources directly to that module and links `dsd::core` privately; no
application static library or separate executable is built. The application
currently registers project information in memory. Keep `Module.cpp` as the sole owner of `DllMain` and
`acrxEntryPoint`.

## Use the adapter as the CMake entry point

Requirements: Windows x64, Visual Studio 2022/v143 with C++ tools, MFC and a Windows
SDK, ObjectARX 2027, CMake 3.24+. Use Release or RelWithDebInfo, shared MFC and /MD.
The supplied SDK finder uses rxapi, acdb26, accore, acad, adui26 and acui26.

From the repository root (the sibling core and application folders are required):

```bat
cmake -S src/adapters/autocad -B build-autocad -G "Visual Studio 17 2022" -A x64 -DOBJECTARX_ROOT="C:/path/to/ObjectARX2027"
cmake --build build-autocad --config Release
```

Or `src\adapters\autocad\build.cmd "C:\path\to\ObjectARX2027"`. The SDK argument defaults to
`ARX`. CMake must be on PATH, or set `CMAKE_EXE` to its full executable path.
The root build writes `src/adapters/autocad/Release/TestARX15.arx` under its build
directory; the adapter-entry build writes `Release/TestARX15.arx`. Use
`-DDUCT_UI_OUTPUT_NAME=YourModuleName` to change its filename.

The legacy direct UI-only build remains available from an **x64 Native Tools
Command Prompt for VS 2022**. It includes the application registration sources,
but does not build or link the core library. Use CMake for the integrated build:

```bat
src\adapters\autocad\build-msvc.cmd "C:\path\to\ObjectARX2027"
```

Output: `src/adapters/autocad/build-msvc/TestARX15.arx`, with compile/link logs. Both build paths
copy `assets/fonts` beside the ARX. Keep that fonts folder with the module.
Fonts load privately into AutoCAD; absent files fall back to Segoe UI.

## Where to make changes

- Fields, sections, spacing and handlers: `include/autocad/ui/panels/*Panel.h`.
- Shared control styling: `include/autocad/ui/controls` and `include/autocad/ui/style/Theme.h`.
- Scroll behavior and section placement: `include/autocad/ui/layout/ScrollPage.h`.
- Horizontal tabs: `include/autocad/ui/layout/TabbedContent.h`.
- Session data: `include/autocad/ui/model` and matching `src/ui/model`.
- Main tab order, ownership and docking: `src/ui/Designer.cpp`.
- Commands: `src/commands/UiCommands.cpp`.
- Host initialization and shutdown: `src/Module.cpp`.

## Regression checks

```bat
cmake -S src/adapters/autocad -B build-autocad-checks -G "Visual Studio 17 2022" -A x64 -DOBJECTARX_ROOT="C:/path/to/ObjectARX2027" -DDUCT_UI_BUILD_CHECKS=ON
cmake --build build-autocad-checks --config Release
ctest --test-dir build-autocad-checks -C Release --output-on-failure
```

Without CMake: `src\adapters\autocad\tests\run-msvc.cmd` in the x64 Native Tools prompt.
Results/captures go to `build-msvc/checks`. Checks compile the actual UI sources
with AutoCAD hosting excluded and exercise defaults, validation, session retention,
tab changes, search/cards, scrolling, resizing and private font loading/fallback.
They return a nonzero exit code on failure. WM_PRINT captures are native control
previews, not AutoCAD screenshots. The harness can later be moved into your root
integration tests; keep it adapter-local until the parent test setup exists.

The project-information checks also exercise application registration, Save/Modify
captions, read-only fields, blank-field saves, and saved state after page recreation.

## Host behavior and ownership

APPLOAD the ARX and run `DUCT_PANEL`. All existing commands are retained:
DUCT_PANEL_CLOSE, PROJECT_INFO_PANEL, NETWORK_PANEL, NETWORK_PANEL_CLOSE,
PROPERTIES_PANEL, DRAWING_GENERATION_PANEL, REPORTS_PANEL and BOQ_PANEL.
NETWORK_PANEL_CLOSE still selects Project Information. Hide/show and tab activation
do not recreate forms or reset values. Picking, generation, persistence and export
remain preview-only operations.

Project Information is connected to `DsdApplication::register_project_information`.
Save accepts all 11 optional fields, registers them in application memory, locks
the edits with `ES_READONLY`, and changes the button to Modify. Modify unlocks
the existing values and restores Save. Edits become registered only on the next
Save. The saved/editing mode survives palette hide/show and page recreation.
The registration and UI state are cleared on module unload. No DWG or disk
persistence is performed. See [project information](../../../docs/project-information.md).

Only Module.cpp defines the extension DLL hooks and includes afxdllx.h. Keep the
host MFC state and use CAcModuleResourceOverride for palette resources. Destroy
child palettes before the palette set, release its private SDK theme afterward,
and destroy all UI windows/fonts before stopping GDI+ and unloading private fonts.
Palette operations run on AutoCAD's UI thread.

The root core build and application source attachment were verified on Linux.
The integrated ARX build and native MFC checks require verification on Windows
with ObjectARX 2027. AutoCAD docking/focus requires an in-app smoke test.
