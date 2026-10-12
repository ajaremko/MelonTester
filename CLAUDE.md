# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About This Project

**MelonTester** is a 16-pad chop sampler plugin by Alfred Young, built to be driven from a Maschine MK2 through a MIDI script in FL Studio: one long sample, each pad plays a region of it, and each pad's region is editable from the controller's knobs. `README.md` is the design document, including what is and isn't implemented yet. It takes MIDI in and has no audio input bus (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`).

This project is derived from the [Pamplejuce](https://github.com/sudara/pamplejuce) template — a JUCE audio plugin template using CMake, C++23, and modern CI/CD. It builds cross-platform (macOS, Windows, Linux) with support for multiple plugin formats (VST3, AU, AUv3, CLAP, Standalone).

The template provides the build system, CI/CD, and project structure. The plugin-specific logic lives in `source/`.

## Build Commands

Development happens in VS Code on Windows, using Ninja and the [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extension. The build directory is `Builds` for both CLI and IDE builds (set `cmake.buildDirectory` to `${workspaceFolder}/Builds` in CMake Tools so they share it).

Ninja needs the MSVC toolchain on the PATH. CMake Tools handles that when a Visual Studio kit is selected. From a plain terminal, load the developer environment first (Visual Studio Build Tools 2026 is installed):

```powershell
# Once per terminal session
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -SkipAutomaticLocation
```

```powershell
# Configure (run once, or after CMakeLists.txt changes)
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build Builds --config Debug

# Run tests (from project root)
ctest --test-dir Builds --verbose --output-on-failure

# Or run tests directly
./Builds/Tests.exe

# Run a single test by name
./Builds/Tests.exe "[test name]"

# Run benchmarks
./Builds/Benchmarks.exe
```

Builds default to **Debug** (faster builds, better debugging). If the user is making music with the plugin and reports audio dropouts or high CPU, build in Release instead: configure with `-DCMAKE_BUILD_TYPE=Release` and build with `--config Release`.

## Project Structure

- `source/` - Plugin source code. Files include each other relative to `source/`, e.g. `"params/ParamIDs.h"`.
  - `PluginProcessor` - owns every module below and wires them together
  - `PluginEditor` - lays out the four panels in `ui/`
  - `params/` - parameter IDs and layout, cached atomic pointers (`ParamRefs`), the selected-pad mirror
  - `sample/` - the loaded sample (`SampleData`) and its hand-off to the audio thread (`SampleStore`)
  - `engine/` - playback: one `PadVoice` per pad, driven by `SamplerEngine`
  - `midi/` - `MidiMap` (JSON-defined note/CC assignments) and `MidiRouter` (MIDI message to pad hit or edit command)
  - `control/` - `EditCommand` + `CommandQueue`, and `EditController`, which applies all edits
  - `slicing/` - `Slicer`: equal division and transient detection as plain functions
  - `ui/` - `GlobalPanel`, `WaveformPanel`, `PadGrid`, `PadParamsPanel`, and the parameter-bound controls in `ParamControls.h`
- `tests/` - Catch2 test files. `tests/helpers/sampler_helpers.h` has helpers for driving the plugin with MIDI.
- `benchmarks/` - Catch2 benchmark files
- `cmake/` - CMake modules (Tests.cmake, Benchmarks.cmake, Assets.cmake, etc.). This is a git submodule, so avoid editing files in it.
- `modules/` - Git submodules: clap-juce-extensions, melatonin_inspector
- `JUCE/` - JUCE framework (git submodule)
- `assets/` - Binary resources (auto-included via juce_add_binary_data)
- `packaging/` - Installer resources and scripts

## Plugin Architecture

State is shared between the two threads; only the message thread changes it.

- **Parameters** (`juce::AudioProcessorValueTreeState`): global settings, 12 settings for each of the 16 pads, and a 12-parameter "selected pad" mirror set (`sel_*`). `SelectedPadMirror` keeps the mirror set equal to the selected pad's own parameters in both directions. The UI and the controller bind to the mirror set. Parameter IDs are saved in user projects, so never rename them. Pad indices are zero-based in code and 1-based in IDs and the `selectedPad` parameter.
- **Sample**: `SampleData` is immutable once loaded. `SampleStore` swaps a new one in for the audio thread at a block boundary and frees the old one on the message thread. The file path is saved in the APVTS state tree (`samplePath`).
- **Audio thread** (`processBlock`): `MidiRouter` translates each MIDI message. Pad hits go straight to `SamplerEngine` so they are sample-accurate. Everything that changes state (selecting a pad, encoder turns, buttons) is pushed onto a `CommandQueue` as an `EditCommand`.
- **Message thread**: `EditController` drains that queue on a 60 Hz timer, applies the edits to parameters, and syncs the mirror. Pad clicks, loading and slicing in the editor call the same `EditController` functions. The on-screen knobs, the MK2 script and host automation write parameters directly instead, and today only `SelectedPadMirror` reacts to those writes (see "One place that reacts to parameter writes" in `TODO.md`). Add new edit behaviour to `EditController`, not to UI components or the audio thread.
- In tests no timer runs, so call `getController().processPending()` after `processBlock` to apply queued edits.
- The MIDI map is JSON: the default is embedded from `assets/midi/default_map.json`, and a user file at `<user app data>/MelonTester/midi_map.json` overrides it. It is read once when the plugin is created.
- **Controller contract**: the Maschine MK2 script (a separate project) reads and writes host parameters, which it finds by their position in the parameter list. Only pad notes (36–51) reach the plugin as MIDI; the script sends no CCs and the plugin sends no MIDI out. So never reorder, insert or remove parameters: add new ones after the last pad parameter. `pad-colors.md` specifies the colour parameters the script reads. The CC side of the MIDI map is for other controllers.

Stubbed features and known rough edges are tracked in `TODO.md`, and `PLAN.md` puts every item into a phase: work through them in that order. When a stubbed feature is finished, tick it in `TODO.md` and update the Status section of `README.md`.

## Build Architecture

**SharedCode Library**: The `SharedCode` INTERFACE library links plugin source code to both the main plugin target and the Tests target, avoiding ODR violations.

**CMake Modules**:
- `PamplejuceVersion.cmake` - Reads VERSION file, optional auto-bump patch level
- `PamplejuceFunctions.cmake` - One include that defines the `pamplejuce_*` functions below (and turns off their legacy include-time behavior)
- `PamplejuceIPP.cmake` - Intel IPP integration (optional, currently disabled: its `include` is commented out in `CMakeLists.txt`)

Target setup happens via explicit function calls in `CMakeLists.txt`, not at include time:
- `pamplejuce_add_assets()` - Includes all files in assets/ as binary data (from `Assets.cmake`)
- `pamplejuce_add_tests()` - Configures the Catch2 test target (from `Tests.cmake`)
- `pamplejuce_add_benchmarks()` - Configures the Catch2 benchmark target (from `Benchmarks.cmake`)
- `pamplejuce_shared_code_defaults()` - C++23, fast math (from `SharedCodeDefaults.cmake`)
- `pamplejuce_xcode_prettify()` - Xcode folder/scheme cleanup (from `XcodePrettify.cmake`)

Note: after `include(PamplejuceFunctions)`, a plain `include(Tests)` (or Assets, etc.) is a no-op - call the function instead.

**Test Discovery**: Uses `catch_discover_tests()` with `PRE_TEST` discovery mode for Xcode compatibility.

## Key Configuration

Set in `CMakeLists.txt`:
- `PROJECT_NAME` - Internal name (no spaces): `MelonTester`
- `PRODUCT_NAME` - Display name in DAWs (can have spaces): `MelonTester`
- `COMPANY_NAME` - Used for bundle name: `Alfred Young`
- `BUNDLE_ID` - macOS bundle identifier and CLAP ID: `com.alfredyoung.melontester`
- `FORMATS` - Plugin formats to build: `Standalone AU VST3 AUv3` (CLAP is added via clap-juce-extensions)
- `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_CODE` - 4-character plugin IDs: `Alfr` / `Mlnt`. Changing these after release breaks existing user sessions.
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `CLAP_FEATURES instrument synthesizer` - instrument configuration

The plugin name is also asserted in `tests/PluginBasics.cpp` and used in `packaging/dmg.json`; update both if it changes.

Version is read from the `VERSION` file in project root.

## CI Status

`.github/workflows/build_and_test.yml` runs the full matrix (Linux, macOS, Windows) on every push and pull request, in Release mode, including pluginval.

Currently disabled, as commented-out blocks in the workflow:
- **Intel IPP**: the install steps and the `IPP_DIR` env var. Re-enable together with `include(PamplejuceIPP)` in `CMakeLists.txt`.
- **macOS code signing and notarization**: the installer `.pkg` is built unsigned. Needs an Apple Developer account. Guide: [How to code sign and notarize macOS audio plugins in CI](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)
- **Windows code signing**: the installer `.exe` is built unsigned. Guide: [Code signing on Windows with Azure Trusted Signing](https://melatonin.dev/blog/code-signing-on-windows-with-azure-trusted-signing/)

Signing is needed before distributing to anyone, since Gatekeeper and SmartScreen block or warn on unsigned plugins.

## Code Quality

Always resolve any compile warnings encountered during builds. Warnings should be treated as errors and fixed before considering a task complete.

Note: LSP/clangd often reports false positive diagnostic errors (like "undeclared identifier", "file not found") because it doesn't have full context of the JUCE module system. Ignore these unless the actual build fails.

## Includes

JUCE modules include common standard library headers (`<vector>`, `<algorithm>`, `<string>`, `<memory>`, etc.) so you don't need to add those explicitly in JUCE code. Adding them is harmless but redundant.

## Threading Model

JUCE plugins have two main threads:

- **Audio thread**: Runs `processBlock` — must be realtime-safe (see below). Never block, allocate, or lock.
- **Message thread**: Runs UI callbacks, parameter listeners, and timer callbacks. Owns the `MessageManager`.

To communicate between them:
- **Simple values**: Use `std::atomic` or JUCE's `AudioParameterFloat`/`AudioParameterBool` (which are atomic under the hood)
- **Larger data**: Use a lock-free queue (e.g. `moodycamel::ReaderWriterQueue`) to pass data from message → audio thread
- **Audio → UI updates**: Use `juce::AsyncUpdater` or `juce::Timer` on the message thread to poll state — never call UI code from the audio thread

## Realtime Safety

For anything in the audio thread / hot DSP path (e.g. `processBlock`):
- Allocate in constructors or `prepareToPlay`, not while rendering audio
- Avoid dynamic allocations and container growth (`std::vector::push_back`, map insertion, string building)
- Prefer fixed-size storage (`std::array`, preallocated buffers, fixed-capacity queues)
- Keep operations deterministic and lock-free where possible

## Adding Dependencies

**JUCE Modules** live in `modules/` as git submodules. Add with `git submodule add`, then `add_subdirectory` and link to `SharedCode` in `CMakeLists.txt`. Some useful ones:

- [melatonin_inspector](https://github.com/sudara/melatonin_inspector) — runtime component debugger (already included)
- [melatonin_blur](https://github.com/sudara/melatonin_blur) — fast cross-platform blurs for C++ UI (shadows, glows, frosted glass)
- [melatonin_perfetto](https://github.com/sudara/melatonin_perfetto) — performance tracing with Perfetto, great for profiling `processBlock` and paint calls
- [gin](https://github.com/FigBug/gin) — large collection of utilities (DSP, UI components, LookAndFeel, etc.)

**Non-JUCE C++ libraries** should be added via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) which is already configured. CPM downloads and caches dependencies at configure time — no submodule needed:

```cmake
CPMAddPackage("gh:nlohmann/json@3.11.3")
target_link_libraries(SharedCode INTERFACE nlohmann_json::nlohmann_json)
```

Some useful CPM libraries:
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing/serialization
- [cameron314/readerwriterqueue](https://github.com/cameron314/readerwriterqueue) — lock-free single-producer/single-consumer queue, ideal for audio↔message thread communication

## Code Style

Uses `.clang-format` with Allman-style braces, 4-space indentation, no column limit.
