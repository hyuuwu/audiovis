# Audio Visualizer

A highly customizable terminal-based audio visualizer built in C, featuring PipeWire audio capture, FFT-based frequency analysis, and extensive configuration options.

## Features

- **PipeWire Audio Capture**: Captures system audio with auto-detection
- **Customizable Visuals**: Multiple gradient modes, custom colors, and bar styles
- **Advanced Audio Processing**: Sensitivity, smoothing, bass boost, frequency range control
- **High Performance**: Configurable FPS, optimized rendering
- **Flexible Layout**: Vertical/horizontal orientation, bar width/spacing control
- **INI Configuration**: Easy-to-edit config file with all options

## Dependencies

Install the required libraries:

```bash
# Debian/Ubuntu
sudo apt install libpipewire-0.3-dev libfftw3-dev libncurses-dev

# Arch Linux
sudo pacman -S pipewire fftw ncurses

# Fedora
sudo dnf install pipewire-devel fftw-devel ncurses-devel
```

## Building

```bash
make
```

## Running

```bash
./audiovis
# or with custom config:
./audiovis /path/to/config.ini
```

Press `q` or `ESC` to quit

## Installing

```bash
sudo make install

```

## Configuration

Edit `config.ini` to customize the visualizer, the file is self-explanatory.
The config file is located in `~/.config/audiovis/config.ini`.

### Audio Settings
- `source`: PipeWire audio source (or "auto" for auto-detection)
- `sample_rate`: Audio sample rate (default: 44100)
- `buffer_size`: Audio buffer size (default: 2048)

### Visual Settings
- `bar_count`: Number of frequency bars (default: 32)
- `bar_char`: Character for bars (default: █)
- `use_colors`: Enable/disable colors (default: 1)
- `gradient_mode`: 0=solid, 1=rainbow, 2=custom (default: 1)
- `color_low/mid/high`: Colors for gradients

### Processing Settings
- `sensitivity`: Overall sensitivity multiplier (default: 1.5)
- `smoothing`: Temporal smoothing 0.0-1.0 (default: 0.7)
- `bass_boost`: Bass frequency boost (default: 1.2)
- `min_freq/max_freq`: Frequency range (default: 20-20000 Hz)

### Performance Settings
- `fps`: Target frames per second (default: 60)
- `sleep_timer`: Sleep when no audio in ms (default: 1000)

### Layout Settings
- `orientation`: 0=vertical, 1=horizontal (default: 0)
- `reverse`: Reverse bar direction (default: 0)
- `bar_width`: Width of each bar in chars (default: 2)
- `bar_spacing`: Spacing between bars (default: 1)

## Examples

### High sensitivity for quiet audio
```ini
[processing]
sensitivity = 3.0
```

### Bass-heavy visualization
```ini
[processing]
bass_boost = 2.0
max_freq = 500
```

### Minimalist horizontal bars
```ini
[visual]
bar_count = 20
bar_char = ▬
use_colors = 0

[layout]
orientation = 1
bar_width = 1
```


GNU GPLv3
