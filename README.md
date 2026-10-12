# MelonTester

MelonTester is a 16-pad sampler plugin for chopping a long sample across the pads, MPC style, without leaving your pad controller. Load one sample, hit a pad, and turn a knob to move where that pad starts and ends.

Most samplers let you trigger slices from hardware but make you reach for the mouse to decide what each pad actually plays. MelonTester exposes each pad's playback region to MIDI, so the whole chopping workflow can be done from the controller. It is designed around a Native Instruments Maschine in MIDI mode, and should work with any controller that has 16 pads and endless encoders.

## Status

Early development. This document describes the intended design; only part of it exists.

Working now:

- Loading a sample by drag and drop or from a file dialog. The 16 pads are spread evenly across it.
- Playing pads from MIDI notes or by clicking them, with per-pad gain.
- Selecting a pad by hitting it, and moving its start and end from MIDI encoders or the on-screen knobs.
- Waveform view with zoom and scroll, showing the selected pad's region.
- All host parameters, and saving and restoring with the project.
- A configurable MIDI map.

Not implemented yet: pitch, attack and release, gate mode, reverse, loop, choke groups and polyphony mode, transient detection, pad colours, and dragging region edges with the mouse. The controls and parameters for these exist but do nothing.

## How it works

1. Load one long audio file: a break, a loop, a whole track.
2. Each of the 16 pads plays its own region of that file.
3. Hit a pad. It plays, and it becomes the pad you are editing.
4. Turn the start and end encoders to move that pad's region. Hit the pad again to hear the result.
5. Repeat across the pads until the sample is chopped the way you want it.

To get a starting point quickly, the sample can be split into 16 equal slices or sliced at detected transients, then adjusted pad by pad.

## Planned features

### Chopping

- **Manual start and end per pad**, anywhere in the sample. Regions can overlap.
- **Equal divisions**: split the sample into 16 equal slices in one action.
- **Transient detection**: place slices on detected hits, with a sensitivity control. When more than 16 hits are found the 16 strongest are kept, assigned to the pads in time order. With fewer than 16, the remaining pads are left blank.

### Per-pad playback

- **One-shot or gate**: play the whole slice, or only while the pad is held.
- **Choke groups**: a pad cuts any other pad playing in the same group.
- **Polyphony mode** for the whole plugin: in Poly only choke groups cut pads; in Mono every new pad cuts whatever is playing.
- **Pitch** and **gain**.
- **Attack and release** to remove clicks at slice edges.
- **Reverse** and **loop**.

### Editor

- Waveform view showing all 16 regions, plus a pad grid.
- Everything that can be done from the controller can also be done with the mouse.
- Load a sample by drag and drop or from a file dialog.

## MIDI control

Pads are played with MIDI notes. Editing uses relative (endless encoder) CCs that always act on the **last pad you hit**, so one small set of controls covers all 16 pads.

| Control | Type | Acts on |
|---|---|---|
| Pads 1–16 | 16 notes | Plays the pad and selects it for editing |
| Zoom | Relative CC | Waveform view |
| View position | Relative CC | Waveform view |
| Slice start | Relative CC | Selected pad |
| Slice end | Relative CC | Selected pad |
| Pitch | Relative CC | Selected pad |
| Gain | Relative CC | Selected pad |
| Attack / release | Relative CC | Selected pad |
| One-shot / gate | Button | Selected pad |
| Reverse | Button | Selected pad |
| Loop | Button | Selected pad |
| Choke group | Relative CC | Selected pad |
| Polyphony mode | Button | All pads |
| Divide equally | Button | Whole sample |
| Detect transients | Button | Whole sample |
| Transient sensitivity | Relative CC | Whole sample |

One step of the start or end encoder moves the edge by 1/512 of what the waveform view shows, so zooming in gives finer control.

### MIDI map

Note and CC assignments are read from a JSON file. The built-in map is [assets/midi/default_map.json](assets/midi/default_map.json): pads on notes 36–51, the controls above on CCs 14–29. These numbers are placeholders until the Maschine template exists.

To use your own, copy that file to `MelonTester/midi_map.json` in your user application data folder (`%APPDATA%` on Windows, `~/Library` on macOS, `~/.config` on Linux) and edit it. It is read when the plugin loads; if it is missing or invalid the built-in map is used.

```json
{
    "channel": 0,
    "relativeEncoding": "twosComplement",
    "padNotes": [36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51],
    "controls": [
        { "cc": 16, "target": "start", "mode": "relative" },
        { "cc": 23, "target": "reverse", "mode": "toggle" },
        { "cc": 28, "target": "divideEqually", "mode": "trigger" }
    ]
}
```

- `channel`: 0 listens on every channel, 1–16 on one.
- `relativeEncoding`: how the encoders report a turn. `twosComplement` (1 right, 127 left), `signedBit` (1 right, 65 left) or `binaryOffset` (65 right, 63 left).
- `padNotes`: exactly 16 note numbers, pad 1 first.
- `target`: `start`, `end`, `pitch`, `gain`, `attack`, `release`, `playMode`, `reverse`, `loop`, `chokeGroup`, `colour`, `polyMode`, `zoom`, `viewX`, `sensitivity`, `divideEqually` or `detectTransients`.
- `mode`: `relative` (endless encoder), `absolute` (0–127 knob or fader), `toggle` or `trigger` (buttons, acting on press).

## Controller feedback

The plugin publishes its state as host parameters so it can be shown outside the plugin window. Every pad has its own set of parameters, which can be automated. On top of that there is one "Selected" set that always shows the last pad you hit: its start and end positions and its playback settings. When you hit a different pad, the Selected parameters update to describe the new pad, and changing one of them changes that pad. Zoom, view position and the selected pad number are parameters too.

## Using it with Maschine

MelonTester is intended to be used with a Maschine controller in **MIDI mode**, with the plugin loaded in any DAW. A Controller Editor template will be provided with the project, mapping the pads to notes and the eight knobs to relative encoders for the controls above.

## Proposed UI

The following is the proposed layout for the UI of the vst. It breaks down to 4 areas:

### Global params
The user can select a polyphony mode here. Buttons to access the equal division and transient detection auto slicing features. These automatically assign slices to all of the 16 pads (or a portion of the pads for a sample with fewer than 16 slices found).

### Waveform viewer/Selection editor
Displays the waveform of the loaded sample. A sample can be dragged from the host DAW and dropped into this area to load a sample. It also displays the boundaries of the current pad's selection of the sample (start and end points). The user can zoom in to the waveform to make finer edits. The area has 4 knobs which control zoom, viewx, start and end. Switching between current pads shows different start and end points per pad but the values of the zoom and viewx controls are global settings. When a pad is selected and its region is outside the view, the view scrolls to it, keeping the current zoom.

### Pads
Displays the 16 pads that can each be assigned a selection of the loaded sample. Each pad can be 
assigned one of 16 possible colors, or white (blank). Also indicates the selected pad.

### Selected pad params
Displays the selected pads params including envelope, pitch, oneshot mode, choke group, etc.

```
┌───────────────────────────────────────────────────────┐
│ ┌──────────┐┌───────────────────────────────────────┐ │
│ │          ││                                       │ │
│ │ global   ││               waveform/selection editor│ │
│ │ params   ││                                       │ │
│ └──────────┘└───────────────────────────────────────┘ │
│ ┌──────────────────────────┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐  │
│ │                          │ │   │ │   │ │   │ │   │  │
│ │                          │ └───┘ └───┘ └───┘ └───┘  │
│ │                          │ ┌───┐ ┌───┐ ┌───┐ ┌───┐  │
│ │                          │ │   │ │   │ │   │ │   │  │
│ │   selected pad params    │ └───┘ └───┘ └───┘ └───┘  │
│ │                          │ ┌───┐ ┌───┐ ┌───┐ ┌───┐  │
│ │                          │ │   │ │   │ │   │ │   │  │
│ │                          │ └───┘ └───┘ └───┘ └───┘  │
│ │                          │ ┌───┐ ┌───┐ ┌───┐ ┌───┐  │
│ │                          │ │   │ │   │ │   │ │   │  │
│ └──────────────────────────┘ └───┘ └───┘ └───┘ └───┘  │
└───────────────────────────────────────────────────────┘
```

## Open questions

- **Display feedback in MIDI mode.** In MIDI mode the Maschine displays show values from the template's own CCs, not host parameters. Keeping the knob readouts in sync with the selected pad may also need the plugin to send CC values over MIDI out. To be tested on hardware.
- **Encoder behaviour.** Which relative encoding Maschine sends in MIDI mode, and whether tying the start and end step size to the zoom level is enough for coarse and fine adjustment when a sample is minutes long.
- **Knob pages.** Zoom, view, start and end use four of Maschine's eight knobs, and the pad settings need more than the remaining four, so the template will need pages.
- **Showing all regions.** The waveform currently highlights the selected pad's region and marks where the other pads start. Overlapping chops may need every region drawn in its pad colour.
- **Blank pads and colours.** Whether a pad has a slice and what colour it is are stored separately, so "white" need not mean "blank".
- **Pad colours on the hardware.** What colour feedback Maschine accepts in MIDI mode is untested; colours are only planned for the plugin window.
- **Dragging from the DAW.** This only works where the host hands over a real file. Browser panes usually do; timeline clips often do not.
- **Very long samples.** Start and end are stored as a fraction of the sample length, which is exact to one sample up to about five minutes at 48 kHz and off by a few samples beyond that.
- **Snap to zero crossings** when moving slice points.
- **Pad banks**, for more than 16 slices of one sample.
- **Time-stretching**, so pitch and length can be changed independently.
- **Embedding audio.** A saved project stores the path to the sample and reports it as missing if the file has moved. Embedding the audio in the project could be added as an option.

## Building

Requires CMake 3.25 or newer and a C++23 compiler. Ninja is recommended.

```bash
git clone --recurse-submodules <repo-url>
cd MelonTester

cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --config Debug
ctest --test-dir Builds --verbose --output-on-failure
```

On Windows, run these from a Visual Studio developer shell so the MSVC toolchain is on the path.

The build produces VST3, CLAP and Standalone versions, plus AU and AUv3 on macOS. Use `-DCMAKE_BUILD_TYPE=Release` and `--config Release` for a build to make music with.

## Credits

Built with [JUCE](https://juce.com/) on the [Pamplejuce](https://github.com/sudara/pamplejuce) template.
