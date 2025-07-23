# Contributing to Vintage Tactical Radio

First off, thank you for considering contributing to Vintage Tactical Radio! It's people like you that make this project such a great tool for the radio enthusiast community.

## Code of Conduct

This project and everyone participating in it is governed by our Code of Conduct. By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues as you might find out that you don't need to create one. When you are creating a bug report, please include as many details as possible:

* **Use a clear and descriptive title** for the issue to identify the problem.
* **Describe the exact steps which reproduce the problem** in as many details as possible.
* **Provide specific examples to demonstrate the steps**.
* **Describe the behavior you observed after following the steps** and point out what exactly is the problem with that behavior.
* **Explain which behavior you expected to see instead and why.**
* **Include screenshots** if possible.
* **Include your system information**:
  * OS and version (e.g., Ubuntu 22.04, Fedora 38)
  * RTL-SDR device model
  * Qt version
  * Any relevant hardware details

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, please include:

* **Use a clear and descriptive title** for the issue to identify the suggestion.
* **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
* **Provide specific examples to demonstrate the steps** or mockups/sketches if it's a UI enhancement.
* **Describe the current behavior** and **explain which behavior you expected to see instead** and why.
* **Explain why this enhancement would be useful** to most Vintage Tactical Radio users.

### Pull Requests

* Fill in the required template
* Do not include issue numbers in the PR title
* Follow the C++ style guide (see below)
* Include thoughtfully-worded, well-structured tests
* Document new code
* End all files with a newline

## Development Setup

1. Fork the repo and create your branch from `main`.
2. Install development dependencies:
   ```bash
   # Debian/Ubuntu
   sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev \
                    librtlsdr-dev libfftw3-dev clang-format

   # Also recommended for development
   sudo apt install gdb valgrind qt6-tools-dev-tools
   ```

3. Build in debug mode:
   ```bash
   mkdir build-debug && cd build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

4. Run tests:
   ```bash
   make test
   ```

## C++ Style Guide

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with some modifications:

### Key Points:

* **Indentation**: 4 spaces (no tabs)
* **Line length**: 100 characters maximum
* **Naming**:
  * Classes: `PascalCase`
  * Functions/Methods: `camelCase`
  * Variables: `camelCase`
  * Constants: `UPPER_SNAKE_CASE`
  * Private members: `memberName_` (with trailing underscore)
* **Headers**: Use `#pragma once` instead of include guards
* **Comments**: Use `//` for single line, `/* */` for multi-line
* **File names**: `lowercase_with_underscores.cpp`

### Example:
```cpp
#pragma once

#include <memory>
#include <string>

namespace vtr {

class ExampleClass {
public:
    ExampleClass(const std::string& name);
    ~ExampleClass() = default;
    
    void doSomething();
    int getValue() const { return value_; }
    
private:
    std::string name_;
    int value_;
    
    static constexpr int DEFAULT_VALUE = 42;
};

} // namespace vtr
```

### Formatting

Use clang-format with our `.clang-format` configuration:
```bash
clang-format -i src/**/*.cpp src/**/*.h
```

## Testing

* Write unit tests for new functionality
* Ensure all tests pass before submitting PR
* Test with actual RTL-SDR hardware if possible
* Test on multiple Linux distributions if feasible

## Documentation

* Update the README.md if needed
* Add inline documentation for complex algorithms
* Update the CHANGELOG.md following [Keep a Changelog](https://keepachangelog.com/)
* Document any new dependencies

## Project Structure

```
vintage-tactical-radio/
├── src/
│   ├── core/        # Core functionality (RTL-SDR, DSP engine)
│   ├── audio/       # Audio output and processing
│   ├── ui/          # User interface components
│   ├── dsp/         # Digital signal processing
│   └── config/      # Configuration and settings
├── tests/           # Unit tests
├── docs/            # Documentation
├── assets/          # Resources (images, sounds, themes)
└── packaging/       # Distribution files
```

## Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally after the first line
* Consider starting the commit message with an applicable emoji:
  * 🎨 `:art:` when improving the format/structure of the code
  * 🐛 `:bug:` when fixing a bug
  * 🔥 `:fire:` when removing code or files
  * 📝 `:memo:` when writing docs
  * 🚀 `:rocket:` when improving performance
  * ✨ `:sparkles:` when introducing new features

## Questions?

Feel free to open an issue with the "question" label or reach out to the maintainers directly.

Thank you for contributing! 📻
