
# Easy Embedded Framework

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://github.com/QuangHaiNguyen/EasyEmbeddedFramework/actions/workflows/linux_build.yaml/badge.svg)](https://github.com/QuangHaiNguyen/EasyEmbeddedFramework/actions)
[![Docs](https://img.shields.io/badge/docs-latest-blue.svg)](docs/index.rst)

## Overview

**Easy Embedded Framework (EZ Framework)** is a lightweight, vendor-neutral, and modular framework designed to simplify embedded system development. It acts as the glue between application logic, hardware-dependent code (drivers, startup), and third-party libraries (RTOS, file systems, etc.), enabling rapid prototyping and robust production firmware.

---

## Features

- **Hardware Abstraction Layer (HAL):** Unified APIs for GPIO, UART, I2C, SPI, and more.
- **Middleware Support:** OS abstraction, file system (TBA).
- **services Support:** event bus, rpc, cli, data model, and more
- **RTOS Integration:** Out-of-the-box support for FreeRTOS, ThreadX, and bare-metal.
- **Extensible Drivers:** Easily add or swap hardware drivers.
- **Cross-Platform:** Build and test on Linux, WSL, or real hardware (STM32, Pi Pico, etc.).
- **Comprehensive Testing:** Modular unit tests and code coverage.
- **Documentation:** Sphinx-based docs and Doxygen API reference.
- **Docker Support:** Reproducible development environment.

---

## Table of Contents

- [Getting Started](#getting-started)
- [Building](#building)
- [Running Tests](#running-tests)
- [Documentation](#documentation)
- [Project Structure](#project-structure)
- [Integration Guide](#integration-guide)
- [Contributing](#contributing)
- [License](#license)
- [Roadmap](#roadmap)

---

## Getting Started

### Prerequisites

- Linux or WSL
- CMake ≥ 3.25
- GCC, Make
- Python 3
- Doxygen, Graphviz, gcovr (for docs/coverage)

Install dependencies:
```bash
sudo apt-get install build-essential cmake doxygen graphviz gcovr python3
```

### Clone the Repository

```bash
git clone https://github.com/QuangHaiNguyen/EasyEmbeddedFramework.git
cd EasyEmbeddedFramework
```

---

## Building

### CMake Presets

List available build presets:
```bash
cmake --list-presets
```

Recommended build (e.g., ThreadX on Linux):
```bash
cmake --preset=linux_threadx_debug
cmake --build --preset=linux_threadx_debug
```

### Running the Example Target

```bash
./build_linux_threadx_debug/targets/linux_threadx/ez_target
```

### Docker (Optional)

Build and run in Docker:
```bash
make docker_build_image
make docker_run_bash
```
See all commands:
```bash
make help
```

---

## Running Tests

Build and run all tests:
```bash
cmake --preset=linux_test_debug
cmake --build --preset=linux_test_debug
ctest --preset=linux_test_debug
```

### Code Coverage

```bash
cmake --build --preset=linux_test_debug --target=coverage
```

---

## Documentation

- **API Docs:** Generated with Doxygen
- **User/Dev Docs:** Sphinx (see `docs/`)

Generate documentation:
```bash
cmake --build --preset=linux_test_debug --target=doxygen
```

---

## Project Structure

```
EasyEmbeddedFramework/
├── src/            # Framework source code (HAL, middleware, services, utilities)
├── inc/            # Public headers
├── tests/          # Unit and integration tests
├── targets/        # Example target projects (Linux, STM32, etc.)
├── extern/         # Third-party libraries (FreeRTOS, littlefs, etc.)
├── tools/          # Code generation and helper scripts
├── docs/           # Documentation (Sphinx, Doxygen)
├── dockerfile      # Docker environment
├── CMakeLists.txt  # Top-level build script
└── ...
```

---

## Integration Guide

To add EZ Framework to your project:

1. Add as a subdirectory in your CMake project:
	```cmake
	add_subdirectory(EasyEmbeddedFramework)
	```
2. Link the library:
	```cmake
	target_link_libraries(YOUR_PROJECT PRIVATE easy_embedded_lib)
	```

See [docs/system_overview.rst](docs/system_overview.rst) and [docs/system_architecture.rst](docs/system_architecture.rst) for more details.

---

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) (if available) or open an issue/PR.

- Bug reports and feature requests: [GitHub Issues](https://github.com/QuangHaiNguyen/EasyEmbeddedFramework/issues)
- Pull requests: Please use the provided templates in `.github/PULL_REQUEST_TEMPLATE/`

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Roadmap

### V1.0.0
- Complete documents
- Refactor


---

## References

- TBD

