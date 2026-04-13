!include "MUI2.nsh"

Name "QuickSnapAudio"
OutFile "QuickSnapAudio-Setup.exe"
InstallDir "$PROGRAMFILES64\QuickSnapAudio"
InstallDirRegKey HKCU "Software\QuickSnapAudio" "InstallDir"
RequestExecutionLevel admin

!define MUI_ICON "resources\icon.ico"
!define MUI_UNICON "resources\icon.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

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
    
    ; Desktop shortcut
    CreateShortCut "$DESKTOP\QuickSnapAudio.lnk" "$INSTDIR\QuickSnapAudio.exe"
    
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
        "DisplayVersion" "1.0.4"
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
