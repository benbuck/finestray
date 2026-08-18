# Change Log

All notable changes to Finestray will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Change history

## [Unreleased]

## [0.5] - 2026-08-17

- Add more script calls to UpdateVersion.ps1
- Remove documented apis from WindowsUndocumented.h
- ContextMenu visible windows current desktop check
- Remove win10 shortcut TODO item
- Don't run clang analyze builds
- Add zed config files
- Use explicit c++ 23 standard
- Fix launch failure with clang build
- Update cJSON to v1.7.19
- Add .codebook.toml for spell checking
- Split cppcheck-all.bat from analyze-all.bat
- Fix clang-tidy issues
- Add clang-tidy scripts
- Minor build script improvements
- Minor cmake improvements
- Add --parallel to all cmake builds
- Minor markdown file formatting fixes
- Fix icons for UWP and packaged apps
- Add error handling to WindowsTracker start()
- Support tray minimize for UWP windows
- Add .cache dir to .gitignore
- Fix some clang analyzer issues
- Add .editorconfig
- Improve stealth window detection
- Restore polling
- Fix noexcept issues
- Fix const-ness issues
- Add analyze-all.bat
- Fix missing debug print newlines
- Improve minimize event debug logging
- Fix buffer allocation logic in Log.cpp
- Add clang crash and settings window TODO items
- Minor debug print improvements
- Better window tracking for add/change events
- Improve last error handling
- Move getWindowGext() helper to WindowInfo::getTitle()
- Minor debug print improvements
- Remove auto-update from TODO list
- Minor debug print adjustments
- Remove winget github workflow, update release checklist
- Link to correct version of README for help
- Split app data from Finestray.rc to AppInfo.h

## [0.4] - 2025-05-21

- Remove poll interval setting
- Remove show windows in menu setting
- Add minimize persistence auto-tray setting
- Add event type auto-tray setting
- Improve method for spy window selection
- Better regular expression checking
- Add version to settings file
- Don't assert in Log::printf if file handle closed
- Add sha256 artifacts of portable and installer
- Avoid using strncpy_s in two cases, and fix third
- Change writeable dir search order
- Improve context menu icon creation
- Refactor and simplify Hotkey parsing and handling
- More debug print logging
- Handle long strings in debug log printf
- Fix icon background color for github builds
- Various internal improvements

## [0.3] - 2025-03-13

- Fix bug in menu hotkey handling that caused an error message
- Add feature to list windows in menu so they can be minimized to the tray
- Add hotkey and menu item to minimize all windows
- Add hotkey and menu item to restore all tray icons
- Add winget package support
- Avoid msvc runtime installation requirement
- Improve error messages
- Add settings validation to improve feedback
- Add reset button to settings dialog
- Use consistent location for Finestray settings and log files
- Add installation section to README
- Fix bug in hotkey handling for "right"
- Handle failed attempts to minimize windows with elevated privileges
- Improve handling of failed main tray icon creation when taskbar doesn't exist
- Use better default path for installer
- Various internal improvements

## [0.2] - 2025-03-01

- Installer runs for current user (issue #3)
- Add hotkey to show context menu (issue #1)
- Show settings dialog on launch if no settings file found
- Remove command line option support
- Add setting to create log file
- Handle windows being restored by other programs
- Various internal improvements

## [0.1] - 2025-01-19

- Initial public pre-release

[unreleased]: https://github.com/benbuck/finestray/compare/v0.5...HEAD
[0.5]: https://github.com/benbuck/finestray/compare/v0.4...v0.5
[0.4]: https://github.com/benbuck/finestray/compare/v0.3...v0.4
[0.3]: https://github.com/benbuck/finestray/compare/v0.2...v0.3
[0.2]: https://github.com/benbuck/finestray/compare/v0.1...v0.2
[0.1]: https://github.com/benbuck/finestray/releases/tag/v0.1

## Legal

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Finestray is distributed under the Apache License, Version 2.0, please see the [license](LICENSE) for more information.

Please see the [privacy policy](PRIVACY.md) for information about privacy concerns.
