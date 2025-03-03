MyFunction(title) {
    DetectHiddenWindows true
    ; MsgBox "The active window's ID is " WinExist(title)
    WinExist(title)
    WinShow(title)
    WinRestore(title)
    WinShow(title)
}

try
    MyFunction("Untitled - Notepad")
catch as e
    MsgBox(type(e) " in " e.What ", which was called at line " e.Line)

