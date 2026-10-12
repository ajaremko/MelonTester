# Implementation plan

The work in [TODO.md](TODO.md), put in order. There are seven phases. Each one leaves the plugin working and ends with something you can check.

Item names here match `TODO.md` exactly. `TODO.md` has the detail and the tick boxes; this file only says what to do when.

Sizes are rough: **S** is contained in one or two files, **M** touches several files or has some design left, **L** is new behaviour with real design still to do. **You** marks an item that needs FL Studio, the Maschine MK2, or a decision only you can make.

## Overview

| Phase | Goal | Items |
|---|---|---|
| 0. Know where things stand | Confirm what exists works, and make the controller contract safe to build on | 6 |
| 1. Chop accurately from the controller | From the MK2, edges move in coarse and fine steps, can't cross, snap to zero crossings, and show as time | 8 |
| 2. Make it sound right | No clicks, and every per-pad playback control does what it says | 9 |
| 3. Finish the plugin window | Everything can be seen and done with the mouse, and the plugin says when something goes wrong | 6 |
| 4. Bank B | 32 pads in two banks | 1 |
| 5. Long samples and automatic slicing | Whole tracks stay responsive, and Detect transients works | 4 |
| 6. Before anyone else uses it | Ready to hand to another person | 4 |
| Later | Not scheduled | 1 |

## Why this order

- **Checks come first** because they are cheap and their results could change everything after them.
- **The controller phase is early** because it fixes the parameter positions your script depends on, and because anything that needs the hardware has the slowest feedback loop.
- **Sound comes before the window** because clicks are the scaffold's most audible flaw, and the window already does enough to work with.
- **Bank B comes after the window** so the pad grid and waveform are finished before they have to show two banks. Its parameter positions are fixed much earlier, in Phase 1.
- **Background loading comes before transient detection** because detection on a long sample needs the same way of working off the message thread.

## Phase 0: Know where things stand

Goal: confirm that what exists works, and make the controller contract safe to build on. No new features.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Check the CI run on all three platforms | S–M | Depends on how many Clang warnings turn up |
| 2 | Listen to it, then try it in FL Studio | You | |
| 3 | Check the MK2 script against the plugin | You | |
| 4 | Tests use the machine's MIDI map | S | |
| 5 | Pin the parameter positions in a test | S | Must be in before Phase 1 adds parameters |
| 6 | Version the saved state | S | Must be in before Phase 1 changes the colour parameter |

Items 1–3 are checks and don't block 4–6.

While doing items 2 and 3, answer two questions that Phase 1 depends on:

- **How much does the script add to a parameter per knob click?** The nudge parameters need it as their step unit.
- **Does FL Studio record the plugin's own parameter changes as automation?** Hit pads with automation recording on and see whether the Selected parameters get recorded. This decides whether item 5 of Phase 1 is any work at all.

Ends when: CI passes on all three platforms with no warnings, anything found by ear or on the hardware is written into `TODO.md`, and both questions are answered.

## Phase 1: Chop accurately from the controller

Goal: from the MK2, a pad's edges move in coarse and fine steps, can't cross, snap to zero crossings, and show as time on the display.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Reserve the new parameters' positions | M | Adds 199 parameters that do nothing yet, at positions 209–407 |
| 2 | Colour parameter matches pad-colors.md | S | |
| 3 | One place that reacts to parameter writes | M | Groundwork for items 4 and 6 |
| 4 | Keep start before end on every path | S | |
| 5 | Keep plugin-made parameter changes apart from user edits | ? | Skip if Phase 0 showed FL Studio doesn't record them. Otherwise the size is unknown until it is tried |
| 6 | Nudge parameters for coarse and fine start and end | M | Needs the step amount from Phase 0. You: try fast turning on the hardware |
| 7 | Snap to zero crossings | M | Includes reworking the global panel's layout to fit the switch |
| 8 | Show start and end as time | M | |

For your script:

- After item 1 the positions are final: Full Level 209, Zero Snap 210, Start Coarse 211, Start Fine 212, End Coarse 213, End Fine 214, Bank 215, bank B's pads from 216.
- After item 6, point the Start and End knobs at the nudge parameters and switch between coarse and fine there.
- After item 8, the Selected Start and End text reads as time.

Ends when, on the MK2 in FL Studio: a coarse turn moves an edge by a step that gets smaller as you zoom in, a fine turn moves it a tenth as far, Start stops short of End, coarse moves land on zero crossings while Zero Snap is on and fine moves don't, and the display shows times.

## Phase 2: Make it sound right

Goal: no clicks, and every per-pad playback control does what it says.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | A tail voice per pad | M | Groundwork for items 2–5 |
| 2 | Edge fades, attack and release | M | |
| 3 | Gate mode | S | |
| 4 | Choke groups | S | |
| 5 | Polyphony mode | S | |
| 6 | Full Level switch | S | Its parameter already exists from Phase 1 |
| 7 | Pitch | S | |
| 8 | Reverse | M | |
| 9 | Loop | M | |

There is a natural stopping point after item 6: the plugin is click-free and playable. Pitch, reverse and loop can follow at any time.

Ends when, by ear: hitting, re-hitting, cutting and letting go of pads never clicks, and each control in the pad settings panel audibly does what it says. You: the listening.

## Phase 3: Finish the plugin window

Goal: everything can be seen and done with the mouse, and the plugin says when something goes wrong.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Pad colours in the editor | M | |
| 2 | Show every pad's region on the waveform | S | |
| 3 | Dragging region edges with the mouse | M | Uses the limit and snap from Phase 1 |
| 4 | A status line for messages | S | Groundwork for item 5, and for two items in later phases |
| 5 | Tell the user when a sample fails to load | S | |
| 6 | Locate button for a missing sample | S | |

Ends when: a sample can be chopped from start to finish with the mouse alone, pads show their colours, an unreadable file gives a message, and a project whose sample has moved can be recovered with its chops intact.

## Phase 4: Bank B

Goal: 32 pads in two banks of 16.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Bank B: 16 more pads | L | Touches parameters, engine, MIDI map and the window |

For your script: switch banks with the Bank parameter at 215, send notes 52–67 for bank B's pads, and read bank B's colours from its own parameters, which start at 216.

Ends when: all 32 pads play from notes 36–67, the bank can be switched in the window and from the script, and Divide equally gives 32 slices.

## Phase 5: Long samples and automatic slicing

Goal: whole tracks stay responsive, and Detect transients works.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Waveform redraw cost on long samples | S–M | Measure first. S if it is already fast enough |
| 2 | Load samples off the message thread | L | |
| 3 | Make saving and restoring safe from other threads | M | Done together with item 2 |
| 4 | Transient detection | L | New algorithm, tuned by ear. Built for both banks from the start |

Ends when: a ten-minute file loads without freezing the window, zooming and scrolling stay smooth, and Detect transients on a drum break puts the slices on the hits.

## Phase 6: Before anyone else uses it

Goal: ready to hand to another person.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Choose a licence | You | |
| 2 | Set up code signing | M | You: needs your Apple Developer and Azure accounts |
| 3 | Report an invalid MIDI map | S | Low priority |
| 4 | Reload the MIDI map without restarting the plugin | M | Low priority |

Ends when: a signed build can be given to someone else, with a licence.

## Later

Not scheduled.

| # | Item | Size | Notes |
|---|---|---|---|
| 1 | Time-stretching | L | |

## Working rules

- Tick items in `TODO.md`, not here.
- When a stubbed feature is finished, move it to "Working now" in the README's Status section.
- Every item ends with a build that has no warnings and tests that pass.
- Anything new that turns up goes into `TODO.md` and gets a place in a phase here.
