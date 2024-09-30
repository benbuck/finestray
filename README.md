# MinTray

MinTray is a program that can minimize windows to the tray area (typically bottom right corner) of the Windows desktop.

## Basic Operation

After launching the MinTray application, it will add an icon for itself to your tray.

MinTray supports several features to help you control when to minimize windows to the tray and restore them back to
their normal placement:

- **Minimize hotkey**:
    First select a window, then press the configurable hotkey (typically Alt+Ctrl+Shift+Down) to minimize the window to
    the tray.
- **Restore hotkey**:
    Press the configurable hotkey (typically Alt+Ctrl+Shift+Up), and the most recent window that was minimized to tray
    will be restored to its original location.
- **Override modifier**:
    Press and hold the configurable key combination (typically Alt+Ctrl+Shift), and then click on the minimize button of
    a window to minimize it to the tray.
- **Tray Icons**:
    For any window that has been minimized to an icon in the tray, simply click it to restore it to its original
    location.
- **Auto-trays**:
    This feature allows specific windows to be minimized to the tray as soon as they appear. Please see the Auto-tray
    Settings section for more information.
- **Exiting MinTray**:
    To exit the MinTray application, right click on any of the icons in the tray that belong to MinTray, and select
    Exit.

Note that the Override modifier also has an effect on auto-trays. It overrides the normal auto-tray behavior. For
example, if you are holding down the override modifier keys when a window is created, it will prevent the normal
auto-tray behavior from happening. If you minimize an auto-tray window while holding the override modifier keys, the
window will minimize like a regular window instead of to the tray.

## Settings

To change the default behavior of MinTray, you can modify its settings. To open the settings dialog, right click on any
of the icons in the tray the belong to MinTray, and select Settings.

The available settings correspond to some of the above features. The settings are:

- **Minimize hotkey**:
    This lets you configure the hotkey that is used to minimize a window to the tray. See below for details about hotkey
    setting values.
- **Restore hotkey**:
    This lets you configure the hotkey that is used to minimize a window to the tray. See below for details about hotkey
    setting values.
- **Override modifier**:
    This lets you configure the modifier that is used to override some Mintray behavior. See below for details about
    modifier setting values.
- **Poll interval**:
    MinTray needs to periodically check all the open windows for the auto-tray feature to work. This setting controls
    how frequently that check is done. The value is in milliseconds, so 1000 means a thousand millisecond, which is one
    second, and MinTray will scan the windows once every second. You can increase the value if you think MinTray is
    checking too often and slowing down your system. You can decrease the value if want the auto-tray behavior to happen
    more rapidly. You can also set this to zero to disable scanning, but that will disable the auto-tray behavior when a
    window first opens.
- **Auto-trays**:
    This lets you configure the list of auto-tray values that are used to control automatically minimizing specific
    windows to the tray. Items can be added to the list by pressing the Add button, removed from the list by pressing
    the Delete button, or modified by selecting an item, entering new values into the fields, and pressing the Update
    button. Please see below for details about auto-tray settings.

Settings are automatically stored in a file called "MinTray.json". MinTray first tries to save them in the same location
as the MinTray application, and if that's not possible it saves them to %APPDATA%\\MinTray\\MinTray.json.

### Modifiers and Hotkeys

Modifier choices: `alt`, `ctrl`, `shift`, `win`

Key choices: `back`, `esc`, `f1`, `f2`, `f3`, `f4`, `f5`, `f6`, `f7`, `f8`, `f9`, `f10`, `f11`, `f12`, `f13`, `f14`,
    `f15`, `f16`, `f17`, `f18`, `f19`, `f20`, `f21`, `f22`, `f23`, `f24`, `tab`, `left`, `right`, `up`, `down`, `space`,
    `home`, `end`, `ins`, `del`, and normal letters, numbers, punctuation, etc. except spaces

For the minimize and restore hotkey settings, you can combine a set of modifiers with one a single key. So
for example, you could make a hotkey like this: `alt win esc`, or `ctrl win -`.

Similarly, for the override modifier, you can provide one ore more modifiers using " ". For example you could provide a
modifier like this: `alt`, or `ctrl shift`.

You can also leave a hotkey or modifier empty or specify `none` to disable the it.

### Auto-tray settings

The auto-tray settings are a list of items that each identify a window that you would like to automatically be minimized
to the tray when it opens, and when you minimize it. The complication is that it's often not trivial to identify a
window. Because of this, MinTray provides three possible ways, which can be used in combination, to identify a window:

- **Executable name**:
    This value corresponds to executable the created the window. Provide the full path to the executable, for example
    `C:\Windows\notepad.exe`. This must exactly match the path of the executable that owns the window, or you can leave
    this empty if you don't care which executable created the window.
- **Window class**:
    This value corresponds to the "class" of the window. The class is an internal value, which you can find using the
    spy feature, or with a tool like Window Spy or Spy++. The class you provide must exactly match the internal value,
    or you can leave this empty if you don't care what the class name is.
- **Window title**:
    This typically corresponds to the text at the top of the window in the title bar, or shown in the taskbar. The value
    is provided as a regular expression (<https://en.cppreference.com/w/cpp/regex>). If you aren't familiar with regular
    expressions, they are much too complicated to explain here, but just as an example, to match the Notepad window
    which has a title like "My Document - Notepad", you could use a regular expression like ".*Notepad$".

All of these values can be automatically filled by selecting the Spy button, and following the instructions to select
the window you want to auto-tray.

## Command Line Options

The same settings that can be set in the settings dialog can also be passed on the command line (except for the
auto-tray settings).

These options are:

- `--hotkey-minimize=[hotkey]`
- `--hotkey-restore=[hotkey]`
- `--modifiers-override=[modifiers]`
- `--poll-interval=[milliseconds]`

Please see above for an explanation of values that can be used for each of these.

## Legal

MinTray is distributed under the Apache License, Version 2.0, please see LICENSE.txt for more information..

Copyright &copy; 2020 [Benbuck Nason](https://github.com/benbuck)

Uses cJSON:
https://github.com/DaveGamble/cJSON

Uses Argh!:
https://github.com/adishavit/argh
