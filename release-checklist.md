# Release checklist

- Update images in README.md

- Update CHANGELOG.md
    Add section near top for high level changes
    Add sections at bottom for detailed change logging

- Update version information:

  - Finestray.rc

    - APP_VERSION
    - APP_VERSION_STRING
    - APP_VERSION_STRING_SIMPLE
    - APP_DATE

  - CMakeLists.txt
    - VERSION

  - README.md
    - Installer file name

- Commit above changes

- Build everything

- Test app and installer

- Add git tag
  - git tag v[x.y]

- Push with tag, push branch
    git push origin main:main --tags

- Update release notes:
  - <https://github.com/benbuck/finestray/releases>

- Deploy winget package using wingetcreate

## Legal

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Finestray is distributed under the Apache License, Version 2.0, please see the [license](LICENSE) for more information.

Please see the [privacy policy](PRIVACY.md) for information about privacy concerns.
