# UI/UX Design Specification
## Vintage Tactical Radio - Visual Design Guide

### Design Inspiration

The interface should evoke the feeling of operating authentic military radio equipment from the 1960s-1980s era. Key inspirations include:

- **AN/PRC-77**: Vietnam War-era manpack radio
- **R-390A**: Cold War military receiver
- **Collins KWM-2A**: Military/commercial transceiver
- **RS-6**: Soviet military radio
- **Racal RA17**: British military receiver

### Layout Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  VINTAGE TACTICAL RADIO V1.0          [SIGNAL] [POWER] [ALERT]  │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌────────────────────────────────────┐   │
│  │  FREQUENCY DIAL  │  │        SPECTRUM DISPLAY           │   │
│  │   ╭─────────╮   │  │    Phosphor Waterfall/Scope       │   │
│  │   │ 96.900  │   │  │                                    │   │
│  │   │   MHz   │   │  └────────────────────────────────────┘   │
│  │   ╰─────────╯   │  ┌────────────────────────────────────┐   │
│  └─────────────────┘  │         S-METER                    │   │
│                       │  1  3  5  7  9  +20  +40  +60 dB   │   │
│  ┌─────────────────┐  │  └──────────────▄─────────┘       │   │
│  │   MODE SELECT   │  └────────────────────────────────────┘   │
│  │ AM FM SSB CW    │                                           │
│  └─────────────────┘  ┌────────────────────────────────────┐   │
│                       │      CONTROL PANEL                  │   │
│  ┌─────────────────┐  │  [VOLUME] [SQUELCH] [GAIN] [TUNE] │   │
│  │  BAND SELECTOR  │  │  [FILTER] [AGC] [NB] [NOTCH]      │   │
│  │ SW MW FM VHF    │  └────────────────────────────────────┘   │
│  └─────────────────┘                                           │
└─────────────────────────────────────────────────────────────────┘
```

### Visual Elements

#### 1. Frequency Display
- **Primary**: Large analog dial with logarithmic scale
- **Secondary**: Digital nixie tube or VFD-style readout
- **Tuning**: Dual-speed knob (coarse/fine)
- **Appearance**: Brass or aluminum dial face with engraved markings

#### 2. Meters and Indicators

**S-Meter Design:**
```
┌─────────────────────────────────────────┐
│ S-METER    1  3  5  7  9  +20  +40  +60│
│            └──┴──┴──┴──┴───┴────┴────┘ │
│                    ▄▄▄▄▄▄▄              │
│                   ███████▌              │
│                    ▀▀▀▀▀▀               │
└─────────────────────────────────────────┘
```
- Backlit analog meter with phosphorescent scale
- Needle with realistic physics (damping, overshoot)
- Peak hold indicator (small secondary needle)

**Status Indicators:**
- Round LED-style lamps with lens covers
- Colors: Amber (signal), Green (power), Red (alert)
- Realistic glow and reflection effects

#### 3. Control Elements

**Rotary Knobs:**
- Large machined aluminum appearance
- Knurled edges for grip
- Pointer indicators
- Smooth rotation with detents at key positions
- Shadow and highlight for 3D effect

**Toggle Switches:**
- Military-style bat handle toggles
- Chrome or black finish
- Satisfying click action
- Status LED below each switch

**Push Buttons:**
- Square military-style with guard covers for critical functions
- Momentary and latching types
- Backlit labels
- Mechanical click feedback

### Color Palettes

#### Military Olive (Default)
```
Background:     #3B3B2F (Olive Drab)
Panel:          #4A4A3D (Lighter Olive)
Text:           #F4E6D7 (Cream)
Displays:       #FF6B00 (Amber)
Meters:         #FFD700 (Gold scale)
Indicators:     #00FF00 (Green), #FF0000 (Red)
```

#### Night Operations (Red Filter)
```
Background:     #1A0000 (Dark Red)
Panel:          #2D0000 (Medium Red)
Text:           #FF0000 (Bright Red)
Displays:       #CC0000 (Deep Red)
Meters:         #FF3333 (Light Red)
Indicators:     #FF0000 (All Red)
```

#### Navy Grey
```
Background:     #2C3E50 (Navy Grey)
Panel:          #34495E (Lighter Grey)
Text:           #ECF0F1 (Off White)
Displays:       #00FF88 (Green Phosphor)
Meters:         #00FF88 (Green scale)
Indicators:     #00FF00 (Green), #FF0000 (Red)
```

### Typography

- **Primary Labels**: Military stencil font (e.g., "Gunplay", "Capture it")
- **Frequency Display**: Digital mono-spaced (e.g., "DSEG7 Classic")
- **Panel Text**: Engraved effect with slight shadow
- **Size Hierarchy**: 
  - Main labels: 14pt
  - Sub-labels: 10pt
  - Indicators: 8pt

### Texture and Materials

1. **Metal Surfaces**
   - Brushed aluminum for front panel
   - Black crinkle paint for case
   - Chrome for toggle switches
   - Brass for dial mechanisms

2. **Surface Effects**
   - Subtle scratches and wear marks
   - Dust in crevices (optional aging)
   - Fingerprints on frequently touched areas
   - Paint chips on edges

3. **Glass Elements**
   - Meter faces with slight reflection
   - Frequency window with anti-glare coating
   - LED lenses with diffusion

### Animation and Interaction

#### Mechanical Responses
- **Knob Rotation**: Smooth with momentum, slight resistance
- **Switch Toggle**: Quick snap with bounce-back
- **Button Press**: Depression animation with click
- **Meter Needle**: Realistic physics with damping

#### Visual Feedback
- **Hover States**: Subtle glow or highlight
- **Active Controls**: Brighter illumination
- **Frequency Change**: Smooth dial rotation
- **Signal Lock**: S-meter stabilization

### Spectrum Display Styling

```
┌────────────────────────────────────────┐
│░░░░░░░░░░░░░▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░│ ← Phosphor persistence
│░░░░░░░░░░▓▓▓████████▓▓▓░░░░░░░░░░░░░░│ ← Signal peak
│░░░░░░░░▓▓████████████████▓▓░░░░░░░░░░░│ ← Waterfall history
│░░░░░░▓▓██████████████████████▓▓░░░░░░░│
│▁▁▁▁▁▃▅████████████████████████▅▃▁▁▁▁▁│ ← Current spectrum
└────────────────────────────────────────┘
```

- Phosphor-style persistence effect
- Grid lines like oscilloscope
- Adjustable intensity and decay
- Optional CRT curvature effect

### Accessibility Considerations

1. **High Contrast Mode**
   - Pure white on black
   - Increased font sizes
   - Simplified graphics

2. **Keyboard Navigation**
   - Tab order follows logical flow
   - Hotkeys for all major functions
   - Visual focus indicators

3. **Screen Reader Support**
   - ARIA labels for all controls
   - Frequency announcements
   - Signal strength descriptions

### Responsive Behavior

The application should maintain its aspect ratio and scale appropriately:
- Minimum window size: 800x600
- Optimal size: 1280x960
- Maximum useful size: 1920x1080

Controls should scale proportionally, maintaining the vintage aesthetic at all sizes.

### Sound Design

Authentic mechanical sounds enhance the vintage experience:
- **Knob rotation**: Subtle friction sound
- **Switch toggle**: Mechanical click
- **Button press**: Tactile thunk
- **Frequency change**: Tuning swoosh
- **Band switch**: Relay clunk
- **Static**: Authentic white/pink noise
- **Squelch tail**: Classic FM squelch sound

### Easter Eggs and Details

1. **Boot Sequence**: Vintage vacuum tube warm-up animation
2. **Secret Frequencies**: Hidden stations with period-appropriate content
3. **Morse Code**: Hidden messages in CW mode
4. **Wear Patterns**: Realistic based on control usage
5. **Serial Number**: Unique per installation
6. **Date Code**: Authentic military date format

This design specification provides comprehensive visual guidelines while maintaining the authentic military radio aesthetic throughout the application.