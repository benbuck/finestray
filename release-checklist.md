# Release checklist

- Update images in README.md

- Update CHANGELOG.md
    Add section near top for high level changes
    Add sections at bottom for detailed change logging

- Update version information:

  - AppInfo.h

    - APP_VERSION
    - APP_VERSION_STRING
    - APP_VERSION_STRING_SIMPLE
    - APP_DATE

  - CMakeLists.txt
    - VERSION

  - README.md
    - Installer file name

- Commit above changes

- Build everything:
  - build-all.bat

- Run cppcheck:
  - Finestray.cppcheck

- Test app and installer

- Add git tag
  - git tag v[VERSION]

- Push with tag, push branch
    git push origin main:main --tags

- Update release notes:
  - <https://github.com/benbuck/finestray/releases>

- Deploy winget package
  - Using github workflow
  - Using wingetcreate:
    wingetcreate update Benbuck.Finestray --version [new version] --urls <https://github.com/benbuck/finestray/releases/download/v[old tag]/Finestray-[old version]-win64.exe>
  - manually clone winget-pkgs repo (https://github.com/microsoft/winget-pkgs.git) and add manifests for new version of Benbuck.Finestray
  - Create winget-pkgs github issue
  - Resolve issue with pull request including new manifests

## Legal

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Finestray is distributed under the Apache License, Version 2.0, please see the [license](LICENSE) for more information.

Please see the [privacy policy](PRIVACY.md) for information about privacy concerns.
