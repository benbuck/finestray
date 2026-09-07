# Release checklist

- Update images in README.md

- Update "uses" versions in .github/workflows

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

- Build and check everything:
    - build-all.bat
    - analyze-all.bat
    - cppcheck-all.bat
    - tidy-all.bat

- Test app and installer

- Add git tag
    - git tag v[VERSION]

- Push with tag, push branch
  git push origin main:main --tags

- Update [release notes](https://github.com/benbuck/finestray/releases)

- Deploy winget package
    - Auto-submits winget-pkgs PR on release publish via .github/workflows/publish-winget.yml
        - If it fails, re-run it manually from Actions
        - Run the dry-run workflow (Test winget package) to inspect manifests
        - FINESTRAY_WINGET secret:
            - GitHub classic PAT with `repo` scope
            - Renew at Settings > Developer settings > Tokens (classic)
            - Paste into repo Settings > Secrets > Actions
    - Fallback:
        - wingetcreate update Benbuck.Finestray --version [new version] --urls https://github.com/benbuck/finestray/releases/download/v[new version]/Finestray-[new version]-win64.exe --submit --token [PAT]
    - Manual fallback:
        - Clone winget-pkgs repo (https://github.com/microsoft/winget-pkgs.git)
        - Add manifests for new version of Benbuck.Finestray
        - Create winget-pkgs github issue
        - Resolve issue with pull request including new manifests

## Legal

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Finestray is distributed under the Apache License, Version 2.0, please see the [license](LICENSE) for more information.

Please see the [privacy policy](PRIVACY.md) for information about privacy concerns.
