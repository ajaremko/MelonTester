# MelonTester

MelonTester is a 16-pad sampler plugin for chopping a long sample across the pads, MPC style, without leaving your pad controller. Load one sample, hit a pad, and turn a knob to move where that pad starts and ends.

Most samplers let you trigger slices from hardware but make you reach for the mouse to decide what each pad actually plays. MelonTester exposes each pad's playback region as host parameters and MIDI controls, so the whole chopping workflow can be done from the controller. It is designed around a Native Instruments Maschine MK2 driven by a MIDI script in FL Studio, and should work with any controller that has 16 pads and endless encoders.

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

Everything still to do, including planned features that have no controls yet, is listed in [TODO.md](TODO.md).

## How it works

1. Load one long audio file: a break, a loop, a whole track.
2. Each of the 16 pads plays its own region of that file.
3. Hit a pad. It plays, and it becomes the pad you are editing.
4. Turn the start and end encoders to move that pad's region. Hit the pad again to hear the result.
5. Repeat across the pads until the sample is chopped the way you want it.

To get a starting point quickly, the sample can be split into 16 equal slices or sliced at detected transients, then adjusted pad by pad.

## Planned features

### Chopping

- **Manual start and end per pad**, anywhere in the sample. Regions can overlap, but a region can't be made shorter than a small minimum length.
- **Coarse and fine movement**: a coarse step moves an edge by 1/512 of what the waveform shows and a fine step by 1/5120, so zooming in makes both finer.
- **Snap to zero crossings**, on by default: coarse moves land on the nearest zero crossing, and fine moves place the edge exactly.
- **Equal divisions**: split the sample into 16 equal slices in one action.
- **Transient detection**: place slices on detected hits, with a sensitivity control. When more than 16 hits are found the 16 strongest are kept, assigned to the pads in time order. With fewer than 16, the remaining pads are left blank. Each slice runs until the next one starts.

### Per-pad playback

- **One-shot or gate**: play the whole slice, or only while the pad is held.
- **Choke groups**: a pad cuts any other pad playing in the same group.
- **Polyphony mode** for the whole plugin: in Poly only choke groups cut pads; in Mono every new pad cuts whatever is playing.
- **Velocity** sets the volume of each hit. A Full Level switch for the whole plugin makes every hit full volume.
- **Pitch** and **gain**. Pitch is read when the pad is hit.
- **Attack and release**: every region has a very short fade at both edges so slices never click. Attack lengthens the fade-in. Release applies when a gate pad is let go or a pad is cut.
- **Reverse**.
- **Loop**, for gate pads: the region repeats while the pad is held.

### Editor

- Waveform view showing all 16 regions in their pad colours, with the selected one emphasised, plus a pad grid.
- **Pad colours** from a palette of 16. Pads without a slice are dark.
- Start and end shown as time into the sample.
- Everything that can be done from the controller can also be done with the mouse.
- Load a sample by drag and drop or from a file dialog. Loading spreads the pads evenly across the new sample.
- If a saved project's sample has moved, a Locate button reloads it and keeps the chops.

Slice positions are stored as a fraction of the sample. They are exact to one sample up to about five minutes at 48 kHz, and within a few samples beyond that.

## Control

Pads are played with MIDI notes. Everything else can be reached in two ways, and both act on the **last pad you hit**, so one small set of controls covers all 16 pads.

- **Host parameters.** Every setting is a parameter that a controller script running in the host can read and write. This is how the Maschine MK2 script works; see [Host parameters](#host-parameters).
- **MIDI CCs.** A controller that sends relative (endless encoder) CCs can use the controls below, through the MIDI map.

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

One step of the start or end encoder is a coarse step.

### MIDI map

Note and CC assignments are read from a JSON file. The built-in map is [assets/midi/default_map.json](assets/midi/default_map.json): pads on notes 36–51, pad 1 lowest, which is what the MK2 script sends, and the controls above on CCs 14–29. The CC numbers are arbitrary defaults for other controllers.

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

## Host parameters

The plugin publishes its state as host parameters so it can be shown and changed outside the plugin window. Every pad has its own set of parameters, which can be automated. On top of that there is one "Selected" set that always shows the last pad you hit: its start and end positions and its playback settings. When you hit a different pad, the Selected parameters update to describe the new pad, and changing one of them changes that pad. Zoom, view position and the selected pad number are parameters too.

The parameters keep a fixed order, because controller scripts find them by position: 5 global parameters, then the 12 Selected parameters, then 12 for each pad. New parameters are only ever added at the end.

The plugin does not send MIDI out. A controller shows the plugin's state by reading parameters.

## Using it with Maschine

MelonTester is built to be used in FL Studio with a Maschine MK2 and a MIDI script for it, which is a separate project. The script passes the pads on as notes and uses the plugin's parameters for everything else:

- The eight knobs are Zoom, View, Start, End, Pitch, Gain, Attack and Release. The script's other eight knob assignments are kept for FL Studio's channel rack.
- Holding a modifier turns two of the knobs into Choke group and Transient sensitivity.
- The script switches the Start and End knobs between coarse and fine movement.
- The display shows the Selected parameters as text.
- The pads light in their pad colours, as described in [pad-colors.md](pad-colors.md).
- Divide equally and Detect transients are done in the plugin window.

## Proposed UI

The following is the proposed layout for the UI of the vst. It breaks down to 4 areas:

### Global params
The user can select a polyphony mode here, and switch Full Level and Zero Snap on and off. Buttons to access the equal division and transient detection auto slicing features. These automatically assign slices to all of the 16 pads (or a portion of the pads for a sample with fewer than 16 slices found).

### Waveform viewer/Selection editor
Displays the waveform of the loaded sample. A sample can be dragged from the host DAW and dropped into this area to load a sample. It also displays the boundaries of the current pad's selection of the sample (start and end points), with the other pads' regions shown faintly in their pad colors. The user can zoom in to the waveform to make finer edits. The area has 4 knobs which control zoom, viewx, start and end. Switching between current pads shows different start and end points per pad but the values of the zoom and viewx controls are global settings. When a pad is selected and its region is outside the view, the view scrolls to it, keeping the current zoom.

### Pads
Displays the 16 pads that can each be assigned a selection of the loaded sample. Each pad can be 
assigned one of 16 possible colors. A pad without a slice is shown dark. Also indicates the selected pad.

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

## Building

Requires CMake 3.25 or newer and a C++23 compiler. Ninja is recommended.

```bash
git clone --recurse-submodules https://github.com/ajaremko/MelonTester.git
cd MelonTester

cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --config Debug
ctest --test-dir Builds --verbose --output-on-failure
```

On Windows, run these from a Visual Studio developer shell so the MSVC toolchain is on the path.

The build produces VST3, CLAP and Standalone versions, plus AU and AUv3 on macOS. Use `-DCMAKE_BUILD_TYPE=Release` and `--config Release` for a build to make music with.

## Credits

Built with [JUCE](https://juce.com/) on the [Pamplejuce](https://github.com/sudara/pamplejuce) template.
