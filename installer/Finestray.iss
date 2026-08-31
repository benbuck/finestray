; Finestray Inno Setup script
;
; Build with:
;   iscc installer\Finestray.iss /DAppExePath=... /DIconPath=...
;
; The version is read from the Finestray.exe version resource, so no version
; values are duplicated here.
;
; Copyright 2020 Benbuck Nason
; Licensed under the Apache License, Version 2.0

#define MyAppName "Finestray"

#ifndef AppExePath
#error AppExePath is required
#endif
#ifndef IconPath
#error IconPath is required
#endif

#define AppVersion GetVersionNumbersString(AppExePath)
#define VersionMajor 0
#define VersionMinor 0
#define VersionRev 0
#define VersionBuild 0
#expr GetVersionComponents(AppExePath, VersionMajor, VersionMinor, VersionRev, VersionBuild)
#define AppVersionString Str(VersionMajor) + "." + Str(VersionMinor)

#define MyAppPublisher "Benbuck Nason"
#define MyAppURL "https://github.com/benbuck/finestray"
#define MyAppExeName "Finestray.exe"

[Setup]
; NOTE: this AppId must remain stable between versions
AppId={{951CCB06-3E44-4683-9EC6-F0A5A85699E1}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppVerName={#MyAppName} {#AppVersionString}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
LicenseFile=..\LICENSE
DefaultDirName={localappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
PrivilegesRequired=lowest
OutputDir={#ExtractFilePath(AppExePath)}
OutputBaseFilename=Finestray-{#AppVersionString}-win64
SetupIconFile={#IconPath}
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#AppVersion}
VersionInfoCopyright=Copyright 2020 Benbuck Nason

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: {#AppExePath}; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\Finestray.json"
Type: files; Name: "{app}\Finestray.log"
