# Repository Guidelines

## Project Structure & Module Organization
- Root directory contains C++ source and UI files, including `mainwindow.*`, `designscene.*`, `wallitem.*`, `openingitem.*`, `furnitureitem.*`, `view2dwidget.*`, and `view3dwidget.*`.
- `assets/` stores runtime assets: `assets/icons/` for SVG icons, `assets/models/` for 3D models, and `assets/models/furniture/catalog.json` for the furniture index.
- `styles.qss` defines shared UI styling, while `resources.qrc` holds Qt resource mappings.
- `wd/` contains planning, requirements, and design documentation used during development.
- `build/` is local output from Qt Creator or command-line builds and should stay untracked.

## Build, Test, and Development Commands
- Recommended: open `untitled.pro` in Qt Creator, select the desired kit (MinGW or MSVC), then Build/Run.
- Command line (MinGW): `qmake untitled.pro` then `mingw32-make`.
- Run the app from the build folder: `cd build/<kit-name>` then `./untitled.exe`.
- Clean build artifacts: `mingw32-make clean` or delete the `build/` directory.

## Coding Style & Naming Conventions
- C++17 with Qt APIs; use 4-space indentation and place braces on the next line for function definitions.
- Classes use `PascalCase` (e.g., `DesignScene`), methods use `camelCase` (e.g., `updateGeometry`), and members use the `m_` prefix.
- Keep UI strings and default values close to their owning classes to ease future adjustments.

## Testing Guidelines
- No automated tests are present. Validate changes manually in both 2D and 3D views.
- If adding tests, use Qt Test and name files `*_test.cpp`, documenting how to run them.

## Commit & Pull Request Guidelines
- Recent commits are short, action-oriented, and often written in Chinese; keep the same concise style.
- Keep summaries under ~50 characters and include the feature area or phase when useful.
- PRs should include a brief description, before/after screenshots for 2D/3D changes, and note any new assets or dependencies.

## Configuration & Dependencies
- Assimp is required for 3D model loading; its paths are configured in `untitled.pro` via vcpkg installs.
- When using MSVC kits, ensure required runtime DLLs are accessible next to the executable or on PATH.

## Further Reading
- See `CLAUDE.md` for architecture notes, algorithms, and workflow details.
