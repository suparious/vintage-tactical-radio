# Settings Dialog Implementation

## Overview
The settings have been successfully moved to a separate dialog window to reduce clutter in the main UI. The Settings dialog can be accessed via:
- **File → Settings...** menu item
- Keyboard shortcut: **Ctrl+,** (standard settings shortcut)

## Settings Dialog Layout

### Audio Settings Group
- **Audio Device**: Dropdown to select audio output device
- **Sample Rate**: 44.1 kHz, 48 kHz, 96 kHz, 192 kHz
- **Sample Format**: 16-bit (s16le) or 24-bit (s24le)

### RTL-SDR Settings Group  
- **Bias-T Power**: Checkbox to enable 4.5V power for active antennas
- **PPM Correction**: Spinbox for frequency correction (-100 to +100 ppm)
- **RTL-SDR Sample Rate**: 2.048 MHz, 2.4 MHz (recommended), 2.56 MHz, 3.2 MHz

### General Settings Group
- **Dynamic Bandwidth**: Checkbox to enable automatic bandwidth adjustment
- **Current Bandwidth**: Read-only display showing current bandwidth
- **Reset All to Defaults**: Button to reset all settings

### Dialog Buttons
- **OK**: Apply changes and close
- **Apply**: Apply changes without closing
- **Cancel**: Discard changes and close

## Main Window Changes

### Removed from Main Window
- Audio device selector
- Sample rate/format selectors  
- Dynamic bandwidth checkbox
- Bias-T checkbox
- PPM correction spinbox
- RTL-SDR sample rate selector
- Reset all button

### Added to Main Window
- Settings menu item in File menu
- Bandwidth display moved to status bar (permanent widget)

## Benefits
1. **Cleaner UI**: Main window now focuses on radio operation controls
2. **Better Organization**: Related settings grouped together logically
3. **Standard UX**: Settings in separate dialog follows desktop application conventions
4. **More Space**: Main controls (frequency, volume, squelch, etc.) have more room
5. **Expandability**: Easy to add more settings without cluttering main UI

## Technical Details
- Settings dialog is created on-demand (lazy loading)
- Settings are persisted when Apply or OK is clicked
- All signals properly connected between dialog and main window
- Dialog remembers position between opens
- Changes can be applied without closing dialog (Apply button)

## Usage
1. Start the application
2. Go to **File → Settings...** or press **Ctrl+,**
3. Adjust settings as needed
4. Click **Apply** to test changes or **OK** to apply and close
5. Settings are automatically saved and restored between sessions
