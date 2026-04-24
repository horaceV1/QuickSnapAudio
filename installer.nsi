!include "MUI2.nsh"

Name "QuickSnapAudio"
OutFile "QuickSnapAudio-Setup.exe"
InstallDir "$PROGRAMFILES64\QuickSnapAudio"
InstallDirRegKey HKCU "Software\QuickSnapAudio" "InstallDir"
RequestExecutionLevel admin

!define MUI_ICON "resources\icon.ico"
!define MUI_UNICON "resources\icon.ico"
!define MUI_ABORTWARNING

; Variable holding user choice for desktop shortcut
Var CreateDesktopShortcut
Var CreateDesktopShortcutCheckbox

!include "nsDialogs.nsh"
!include "LogicLib.nsh"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY

; Custom page asking the user whether to create a desktop shortcut
Page custom DesktopShortcutPage DesktopShortcutPageLeave

!insertmacro MUI_PAGE_INSTFILES

; Finish page with "Run app now" option
!define MUI_FINISHPAGE_RUN "$INSTDIR\QuickSnapAudio.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch QuickSnapAudio now"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Function DesktopShortcutPage
    !insertmacro MUI_HEADER_TEXT "Desktop Shortcut" "Choose whether to add a shortcut to your desktop."

    nsDialogs::Create 1018
    Pop $0

    ${NSD_CreateLabel} 0 0 100% 24u "Would you like to create a shortcut for QuickSnapAudio on your desktop?"
    Pop $0

    ${NSD_CreateCheckbox} 0 40u 100% 12u "Create a desktop shortcut"
    Pop $CreateDesktopShortcutCheckbox
    ${NSD_Check} $CreateDesktopShortcutCheckbox

    nsDialogs::Show
FunctionEnd

Function DesktopShortcutPageLeave
    ${NSD_GetState} $CreateDesktopShortcutCheckbox $CreateDesktopShortcut
FunctionEnd

Section "Install"
    SetOutPath "$INSTDIR"
    
    ; Copy all files from the deploy directory
    File /r "deploy\*.*"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Start menu shortcuts
    CreateDirectory "$SMPROGRAMS\QuickSnapAudio"
    CreateShortCut "$SMPROGRAMS\QuickSnapAudio\QuickSnapAudio.lnk" "$INSTDIR\QuickSnapAudio.exe"
    CreateShortCut "$SMPROGRAMS\QuickSnapAudio\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    
    ; Desktop shortcut (only if user opted in on the custom page)
    ${If} $CreateDesktopShortcut == ${BST_CHECKED}
        CreateShortCut "$DESKTOP\QuickSnapAudio.lnk" "$INSTDIR\QuickSnapAudio.exe"
    ${EndIf}
    
    ; Registry
    WriteRegStr HKCU "Software\QuickSnapAudio" "InstallDir" "$INSTDIR"
    
    ; Add/Remove Programs entry
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio" \
        "DisplayName" "QuickSnapAudio"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio" \
        "DisplayIcon" "$INSTDIR\QuickSnapAudio.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio" \
        "Publisher" "Daniel Filipe Leonardo Pessoa"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio" \
        "DisplayVersion" "1.0.7"
SectionEnd

Section "Uninstall"
    ; Remove files
    RMDir /r "$INSTDIR"
    
    ; Remove shortcuts
    RMDir /r "$SMPROGRAMS\QuickSnapAudio"
    Delete "$DESKTOP\QuickSnapAudio.lnk"
    
    ; Remove registry entries
    DeleteRegKey HKCU "Software\QuickSnapAudio"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QuickSnapAudio"
SectionEnd
