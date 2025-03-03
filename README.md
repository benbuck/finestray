# Finestray

Finestray (pronounced fin-es-tray) is a program that can minimize windows to the system tray area (typically bottom
right corner, formally known as the "notification area") of the Windows desktop. This is useful if you want to have a
window not be visible, and also not show in the taskbar (the usual behavior when a window is minimized) in order to
reduce visual clutter.

> [!NOTE]
> This is a pre-release version, so it will likely have problems. Please report any
> [issues](https://github.com/benbuck/finestray/issues) that you find.

## Basic Operation

After launching the Finestray application, it will add an icon for itself to your tray. The icon looks like this:
![Finestray icon](src/images/icon.png "Finestray icon")

Finestray supports a number of features to help you control when to minimize windows to the tray and restore them back
to their normal placement:

- **Minimize hotkey**:
  First select a window, then press the configurable hotkey (typically Alt+Ctrl+Shift+Down) to minimize the window to
  the tray.
- **Restore hotkey**:
  Press the configurable hotkey (typically Alt+Ctrl+Shift+Up), and the most recent window that was minimized to tray
  will be restored to its prior location.
- **Restore all hotkey**:
  Press the configurable hotkey (typically Alt+Ctrl+Shift+Left), and all of the minimized windows will be restored to
  their prior locations.
- **Menu hotkey**:
  Press the configurable hotkey (typically Alt+Ctrl+Shift+Home) to show the [Context menu](#context-menu).
- **Override modifier**:
  Press and hold the configurable key combination (typically Alt+Ctrl+Shift), and then click on the minimize button of a
  window to minimize it to the tray. See the [Auto-tray settings](#auto-tray-settings) section for additional
  functionality.
- **Tray Icons**:
  If you click on the main tray icon for Finestray itself, it will show the [Settings](#settings) window, and if you
  click it again it will hide the Settings window.

  For any window that has been minimized to an icon in the tray, simply click it to restore it to its original location.
- **Auto-trays**:
  This feature allows specific windows to be minimized to the tray as soon as they appear or when they are minimized.
  Please see the [Auto-tray Settings](#auto-tray-settings) section for more information.
- **Icon menu**:
  For any of the tray icons created by Finestray, including its own, you can right click on it to see the [Context menu]
  (#context-menu).

## Context Menu

The context menu is accessible by right clicking on any of the tray icons that Finestray creates, or by activating the
associated hotkey. When shown, the context menu looks like this:

![Context menu](src/images/context-menu.png "Context menu")

From the context menu, you can select one of the menu items:

- **Finestray**:
  Shows a some basic information about Finestray.
- **Restore all**
  Restores all minimized windows to their original states. This menu item won't be available if there are no minimized
  windows.
- **Settings**:
  Shows the [Settings](#settings) window.
- **Exit**:
  Exits the Finestray application.

Additionally, if your `Minimize placement` is set to `Menu` or `Tray and Menu`, then minimized windows will be listed in
the context menu. You can click on these to restore a window to its original location.

## Settings

To change the default behavior of Finestray, you can modify its settings. To open the Settings window, right click on
any of the icons in the tray the belong to Finestray, and select Settings, or just click once on the main Finestray tray
icon.

![Settings window](src/images/settings-window.png "Settings window")

The available settings correspond to some of the above features. The settings are:

- **Start with windows**:
  This toggles whether Finestray is launched when you start Windows. This creates a shortcut link located in your
  startup folder (`shell:Startup`, typically
  `C:\Users\Your-Name\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`) which which launches the Finestray
  executable.
- **Enable log file**:
  This toggles whether Finestray saves logging information to a file or not. If enabled, Finestray first tries to save
  the log file in the same location as the Finestray application, and if that's not possible it saves to
  `%APPDATA%\\Finestray\\Finestray.log`.
- **Minimize placement**:
  This controls where to place minimized windows minimized using Finestray. The choices are:
  - **Tray**: the window will get an icon in the tray.
  - **Menu**: the window will get an entry in the context menu.
  - **Tray and Menu**: the window will get an icon in the tray and an entry in the context menu.
- **Minimize hotkey**:
  This lets you configure the hotkey that is used to minimize a window to the tray. Please see the
  [Modifiers and Hotkeys](#modifiers-and-hotkeys) section for more information.
- **Restore hotkey**:
  This lets you configure the hotkey that is used to restore a window to it's original state. Please see the
  [Modifiers and Hotkeys](#modifiers-and-hotkeys) section for more information.
- **Menu hotkey**:
  This lets you configure the hotkey that is used to show the [Context menu](#context-menu). Please see the
  [Modifiers and Hotkeys](#modifiers-and-hotkeys) section for more information.
- **Override modifier**:
  This lets you configure the modifier that is used to override some Finestray behavior. Please see the
  [Modifiers and Hotkeys](#modifiers-and-hotkeys) section for more information.
- **Poll interval**:
  Finestray needs to periodically check all the open windows for the auto-tray feature to work. This setting controls
  how frequently that check is done. The value is in milliseconds, so `1000` means one thousand milliseconds, which is
  one second, and Finestray will scan the windows once every second. You can increase the value if you think Finestray
  is checking too often and slowing down your system. You can decrease the value if want the auto-tray behavior to
  happen more rapidly. You can also set this to zero to disable scanning, but that will disable the auto-tray behavior
  when a window first opens.
- **Auto-trays**:
  This lets you configure the list of auto-tray values that are used to control automatically minimizing specific
  windows to the tray. Items can be added to the list by pressing the Add button, removed from the list by pressing the
  Delete button, or modified by selecting an item, entering new values into the fields, and pressing the Update button.
  Please see the [Auto-Tray Settings](#auto-tray-settings) section for more information.

At the bottom of the Settings dialog are four buttons: About, Exit, Cancel, and OK:

- **About**:
  Shows a some basic information about Finestray.
- **Exit**:
  Exits the Finestray application after saving settings.
- **Cancel**:
  Closes the Settings window but does not save settings (settings are restored to their unmodified values).
- **OK**:
  Closes the Settings window and saves settings.

Settings are automatically stored in a file called "Finestray.json". Finestray first tries to save them in the same
location as the Finestray application, and if that's not possible it saves them to
`%APPDATA%\\Finestray\\Finestray.json`.

### Modifiers and Hotkeys

Modifier choices: `alt`, `ctrl`, `shift`, `win`

Key choices: `back`, `esc`, `f1`, `f2`, `f3`, `f4`, `f5`, `f6`, `f7`, `f8`, `f9`, `f10`, `f11`, `f12`, `f13`, `f14`,
  `f15`, `f16`, `f17`, `f18`, `f19`, `f20`, `f21`, `f22`, `f23`, `f24`, `tab`, `left`, `right`, `up`, `down`, `space`,
  `home`, `end`, `ins`, `del`, letters, numbers, and punctuation.

For the override modifier, you can provide one or more modifiers using spaces in between. For example you could provide
a modifier `alt`, or a modifier `ctrl shift`.

Similarly, for the minimize and restore hotkeys, you can combine a set of modifiers together with a single key. So for
example, you could combine the modifier `alt win` with the key `esc` to make a hotkey `alt win esc`. Or you could
combine a modifier `ctrl win` with the key `-` to make a hotkey `ctrl win -`.

You can also leave a hotkey or modifier empty or specify `none` to disable it.

### Auto-tray settings

The auto-tray settings are a list of items that each identify a window that you would like to automatically be minimized
to the tray when it opens, and when you minimize it. The complication is that it's often not trivial to identify a
window. Because of this, Finestray provides three possible ways, which can be used in combination, to identify a window:

- **Executable name**:
  This value corresponds to program the created the window. Provide the full path to the executable, for example
  `C:\Windows\notepad.exe`. This must match (case insensitive) the path of the executable that owns the window, or you
  can leave this empty if you don't care which executable created the window.
- **Window class**:
  This value corresponds to the "class" of the window. The class is an internal value, which you can find using the
  [Spy feature](#spy-feature). The class you provide must exactly match the internal value, or you can leave this empty
  if you don't care what the class name is.
- **Window title**:
  This typically corresponds to the text at the top of the window in the title bar, or shown in the taskbar. The value
  is provided as a [regular expression](https://en.cppreference.com/w/cpp/regex). If you aren't familiar with regular
  expressions, they are much too complicated to explain here, but just as an example, to match the Notepad window which
  has a title like "My Document - Notepad", you could use a regular expression like: `.*Notepad$`.

Note that the Override modifier also has an effect on auto-trays. It overrides the normal auto-tray behavior. For
example, if you are holding down the override modifier keys when a window is created, it will prevent the normal
auto-tray behavior from happening. If you minimize an auto-tray window while holding the override modifier keys, the
window will minimize the standard way instead of to the tray.

### Spy feature

If you don't know the executable, class, or title of a window, or just would like help filling those values, you can use
the Spy feature. To use it, first open the Settings window, then press the "Spy" button. This will close the Settings
window, then show you a message explaining that Spy mode is active, and that you can click on any window to select it.
Press the "OK" button of that message, and then click on the window you want to obtain the executable, class, and title
of. Once you have done that, the Settings window should re-appear, and those values should be filled in. You can modify
them if you like, or leave them alone, and then press the Add button to create a new Auto-tray item for that window.

If the Spy feature doesn't work for you, alternatively you can use another tool like
[Window Spy](https://amourspirit.github.io/AutoHotkey-Snippit/WindowSpy.html) or
[Spy++](https://learn.microsoft.com/en-us/visualstudio/debugger/introducing-spy-increment) which provide more advanced
window identification features.

## Limitations

Unfortunately, Finestray can not currently minimize all windows to the tray. Some software creates windows that do not
adhere to the normal behavior for minimizing. For example, apps obtained from the Windows Store may not be compatible
with Finestray. WhatsApp is one popular app that Finestray can't currently minimize to the tray.

## Legal

Copyright &copy; 2020 [Benbuck Nason](<https://github.com/benbuck>)

Finestray is distributed under the [Apache License, Version 2.0](LICENSE).

Uses the [cJSON library](https://github.com/DaveGamble/cJSON) for JSON parsing.

Uses some [Google Noto Emoji](https://github.com/googlefonts/noto-emoji) for image artwork.
