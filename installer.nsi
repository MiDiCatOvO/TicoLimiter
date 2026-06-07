; Tico Limiter Windows Installer
; Requires NSIS: https://nsis.sourceforge.io/Download

!define PRODUCT_NAME "Tico Limiter"
!define PRODUCT_PUBLISHER "KawaiiAudio"
!define PRODUCT_VERSION "1.0.0"

Name "${PRODUCT_NAME}"
OutFile "TicoLimiter-Setup.exe"
InstallDir "$COMMONFILES\VST3"
RequestExecutionLevel admin

; Pages
Page directory
Page instfiles

Section "VST3 Plugin" SecVST3
    SetOutPath "$INSTDIR\${PRODUCT_NAME}.vst3"
    File /r "build\TicoLimiter_artefacts\Release\VST3\${PRODUCT_NAME}.vst3\*.*"
SectionEnd

Section "Standalone" SecStandalone
    SetOutPath "$INSTDIR\..\..\KawaiiAudio\${PRODUCT_NAME}"
    File "build\TicoLimiter_artefacts\Release\Standalone\${PRODUCT_NAME}.exe"
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\..\..\KawaiiAudio\${PRODUCT_NAME}\${PRODUCT_NAME}.exe"
SectionEnd

Section "Uninstall"
    RMDir /r "$COMMONFILES\VST3\${PRODUCT_NAME}.vst3"
    RMDir /r "$INSTDIR\..\..\KawaiiAudio\${PRODUCT_NAME}"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
SectionEnd
