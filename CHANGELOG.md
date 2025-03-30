# Change Log

All notable changes to Finestray will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Change history

## [Unreleased]

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

[unreleased]: https://github.com/benbuck/finestray/compare/v0.3...HEAD
[0.3]: https://github.com/benbuck/finestray/compare/v0.2...v0.3
[0.2]: https://github.com/benbuck/finestray/compare/v0.1...v0.2
[0.1]: https://github.com/benbuck/finestray/releases/tag/v0.1

## Legal

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Finestray is distributed under the Apache License, Version 2.0, please see the [license](LICENSE) for more information.

Please see the [privacy policy](PRIVACY.md) for information about privacy concerns.
