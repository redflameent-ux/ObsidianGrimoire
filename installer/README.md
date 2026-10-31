# Installer - The TRANCE-6 Obsidian Grimoire

`ObsidianGrimoire.iss` is an [Inno Setup](https://jrsoftware.org/isdl.php) script that
packages all eight plugins into one installer, `TRANCE-6-Obsidian-Grimoire-Setup.exe`.
It installs each plugin's **VST3** to the shared `Common Files\VST3` folder every DAW
scans, and each **stand-alone** app under Program Files, with a per-plugin checkbox so
users can install the whole set or any subset.

## Build the installer (Windows)

1. **Build the plugins in Release** (from the repo root), so the artefacts exist at
   `build\<PLUGIN>_artefacts\Release\`:
   ```
   cmake -S . -B build
   cmake --build build --config Release
   ```
2. **Install Inno Setup 6** (free): https://jrsoftware.org/isdl.php
3. **Compile the installer** - double-click `installer\ObsidianGrimoire.iss` and press
   *Build*, or from a terminal:
   ```
   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\ObsidianGrimoire.iss
   ```
   The result lands in `installer\Output\TRANCE-6-Obsidian-Grimoire-Setup.exe`.

## Distributing it

Attach `TRANCE-6-Obsidian-Grimoire-Setup.exe` to a GitHub Release so people download the
whole set as one file:
```
gh release create v1.0.0 installer/Output/TRANCE-6-Obsidian-Grimoire-Setup.exe -t "The TRANCE-6 Obsidian Grimoire v1.0.0"
```
