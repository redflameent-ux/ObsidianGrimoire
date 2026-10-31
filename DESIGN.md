# RedFlame UI kit - design spec (round two)

The shared visual language for the Red Flame Entertainment line (ORACLE, RESONA,
DEEPFIELD, TENFOLD, EMBER, FURROW, SILTH, CLEARING). Everything here lives in
`Source/redflame/` and is header-only so every plugin pulls the same kit.

## Colour tokens (`rf::` in `RedFlameTheme.h`)

| token        | hex        | use                                   |
|--------------|------------|---------------------------------------|
| `bg`         | `#060709`  | page background (near-black, bluish)  |
| `panel`      | `#111216`  | card fill                             |
| `panelHi`    | `#191a20`  | raised control fill (buttons, combos) |
| `panelLo`    | `#0b0c0f`  | inset wells / displays                |
| `border`     | `#24252c`  | card + control outline                |
| `hairline`   | `#1b1c22`  | faint dividers                        |
| `text`       | `#e8e9ee`  | primary label                         |
| `textDim`    | `#8a8c96`  | value / secondary                     |
| `textFaint`  | `#55575f`  | tertiary / disabled                   |

Brushed metal cap: `metalHi #595b63` (top-left) -> `metalMid #2b2c32` -> `metalLo #141418`,
bright rim `metalRim #6a6c74`.

### Accents (one per plugin)
`accOracle #2f6bff` (royal), `accResona #8f9bf0`, `accDeepfield #9b7cff`,
`accTenfold #ff7a3c`, `accEmber #3f9bff`, `accFurrow #e3b23e`, `accClearing #d9b24a`.

### Modulation-source families (bottom rail)
`modEnv #e3a73e` (gold), `modLfo #35c6e8` (cyan), `modMacro #9b84ff` (violet),
`modMidi #f2497e` (pink: velo / wheel / press / key / random). These carry across the
whole line unchanged - they align with the TRANCE-6 era palette for brand continuity.

## Metrics (`rf::m::`)
card radius 10, control radius 5, card padding 14, header strip 26, top bar 56,
mod rail 46, keyboard 112, knob label/value 15 each.

## Components (`RedFlameWidgets.h`, `RedFlameTopBar.h`, `RedFlameModRail.h`)

- **RedFlameLookAndFeel** - the house `LookAndFeel_V4`. The signature piece is
  `drawRotarySlider`: drop shadow, a 44-tick radial ring (lit portion in accent), a
  dim track arc, a bright accent value arc (bipolar knobs fill from centre), a
  brushed-metal cap (radial gradient + machined rim + specular highlight) and a pointer.
  Also pill `drawToggleButton`, flat `drawButtonBackground` (accent when toggled),
  `drawComboBox`. Set the accent with `setAccent()` per plugin.
- **rf::Panel** - the dark card with an accent header tick + uppercase title. Add
  controls to it and lay them out inside `content()`.
- **rf::Knob / Toggle / Choice / VSlider** - APVTS-bound control + caption (+ value).
  `Knob` is also a drag-and-drop mod target (see below).
- **rf::Segmented** - a button row bound to a choice/int parameter (e.g. filter type).
- **rf::TopBar** - brand lockup, plugin wordmark, page tabs, preset browser,
  SAVE/MIDI/RANDOMIZE/HELP, master knob + meter.
- **rf::ModRail / rf::Chip** - the bottom rail; chips coloured + glyphed by family,
  and each chip is a drag source (`startDragging("modsrc:<n>")`).

## Modulation (`RedFlameMod.h`)
- **rf::ModMatrix** - the source->dest->depth store, serialised to/from a `MODMATRIX`
  ValueTree child of the plugin state (persists with presets).
- **rf::ModDot** - a draggable dot that lives on a `Knob`'s arc at the modulated
  position; drag = depth, right-click = remove.
- **Drag to assign**: drop a rail chip onto any knob -> an assignment is added and a
  coloured dot (by source family) appears on the arc, exactly like the multi-source
  cutoff knob in the ORACLE render. The editor is a `DragAndDropContainer` and rebuilds
  all knobs' dots on any add/remove/preset-load.

## Not yet built (next passes)
- Applying the mod matrix to DSP (assignments are stored + shown; audio-rate modulation
  of destinations is the next step). ENV 1 and global tune already drive the proof synth.
- Live visualizers - `Display`, `EnvDisplay` and `LfoDisplay` are stylized/animated
  stand-ins (EnvDisplay does read the live A/H/D/S/R; LfoDisplay reads the live shape).
- INSTRUMENT (walnut) skin for ORACLE; the MOD / ARP / FX pages.
- The real wavetable / harmonic / filter DSP (the processor is a small proof synth).
