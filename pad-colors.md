# MelonTester: sending pad colours to the Maschine MK2 script

The Maschine MK2 script for FL Studio lights each of its 16 pads in the colour of the matching MelonTester pad. This describes what the plugin needs to expose for that to work.

## Summary

Each pad's colour is an index into a fixed palette of 16 colours. The plugin exposes it through the per-pad parameter it already lists, "Pad N Colour", and through "Selected Colour" for the selected pad. The script reads those parameters. Nothing is sent over MIDI.

## Why a parameter

An FL Studio MIDI script can read a VST's automatable parameters, each as a normalised float from 0 to 1. It can't see the plugin's GUI state, and FL doesn't deliver a plugin's MIDI output to a controller script. So a parameter is the only direct route.

## Encoding

- **Type:** a stepped (discrete) parameter with 16 steps.
- **Value:** normalised value = index / 15, where index is 0–15.
- **Decoding:** the script uses round(value × 15), so small float errors from the host don't matter.
- **Display text:** your choice (a number or a colour name). The script doesn't read it.

## Parameter positions

The script finds parameters by their index in the plugin's parameter list, so these positions must not move:

| Parameter | Index |
|---|---|
| Selected Colour | 15 |
| Selected Assigned | 16 |
| Pad N Colour (N = 1–16) | 17 + 12 × (N − 1) + 10 |
| Pad N Assigned (N = 1–16) | 17 + 12 × (N − 1) + 11 |

For example, Pad 1 Colour is 27, Pad 2 Colour is 39 and Pad 16 Colour is 207. This is the layout the plugin reports today: 17 global parameters, then 12 per pad.

If the layout has to change, the script's indices must change with it.

## The palette

Sixteen hues, 22.5° apart, at 85% saturation and 90% brightness. Draw the pads in these colours in the plugin's GUI so the screen and the hardware match.

| Index | Value (index / 15) | RGB | Hue |
|---|---|---|---|
| 0 | 0.0000 | `#E62222` | 0° (red) |
| 1 | 0.0667 | `#E66C22` | 22.5° |
| 2 | 0.1333 | `#E6B522` | 45° |
| 3 | 0.2000 | `#CDE622` | 67.5° |
| 4 | 0.2667 | `#84E622` | 90° |
| 5 | 0.3333 | `#3BE622` | 112.5° |
| 6 | 0.4000 | `#22E653` | 135° |
| 7 | 0.4667 | `#22E69C` | 157.5° |
| 8 | 0.5333 | `#22E6E6` | 180° (cyan) |
| 9 | 0.6000 | `#229CE6` | 202.5° |
| 10 | 0.6667 | `#2253E6` | 225° |
| 11 | 0.7333 | `#3B22E6` | 247.5° |
| 12 | 0.8000 | `#8422E6` | 270° |
| 13 | 0.8667 | `#CD22E6` | 292.5° |
| 14 | 0.9333 | `#E622B5` | 315° |
| 15 | 1.0000 | `#E6226C` | 337.5° |

This is the same palette the script uses for recolouring FL's channels, mixer tracks and patterns from the MK2. The palette lives in both the plugin and the script: changing the number of colours or any RGB value means changing both.

## Behaviour the script relies on

- **Selected Colour mirrors the selected pad.** It reads as that pad's colour, and writing it changes that pad's colour (and its "Pad N Colour"). This lets the MK2 recolour the selected pad.
- **Assigned decides whether a pad lights.** "Pad N Assigned" at 0.5 or above means the pad has a slice. Unassigned pads are dark on the MK2 whatever their colour.
- **Tell the host about changes.** When a colour or assignment changes in the plugin's GUI, notify the host as for any parameter change, so the hardware follows without waiting for the next pad hit.
- **Default colours.** Give new pads a default so they aren't all red. Suggested: pad N starts at index N − 1.

## Alternatives not chosen

- **RGB packed into one float:** 24 bits is at the limit of a 32-bit float, so colours come back slightly wrong.
- **Three parameters per pad (R, G, B):** exact, but adds 32 parameters per bank.
- **Hue only:** no shared palette needed, but the GUI and the hardware colours don't match.
