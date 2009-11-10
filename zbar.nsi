#------------------------------------------------------------------------
#  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the ZBar Bar Code Reader.
#
#  The ZBar Bar Code Reader is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The ZBar Bar Code Reader is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the ZBar Bar Code Reader; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zbar
#------------------------------------------------------------------------

!ifndef VERSION
  !define VERSION "test"
!endif
!ifndef PREFIX
  !define PREFIX "\usr\mingw32\usr"
!endif

!define ZBAR_KEY "Software\ZBar"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZBar"

OutFile zbar-${VERSION}-setup.exe

SetCompressor /SOLID bzip2

InstType "Typical"
InstType "Full"

InstallDir $PROGRAMFILES\ZBar
InstallDirRegKey HKLM ${ZBAR_KEY} "InstallDir"

!define SMPROG_ZBAR "$SMPROGRAMS\ZBar Bar Code Reader"

Icon ${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico
UninstallIcon ${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico

# do we need admin to install uninstall info?
#RequestExecutionLevel admin


!include "MUI2.nsh"
!include "Memento.nsh"


Name "ZBar"
Caption "ZBar ${VERSION} Setup"


!define MEMENTO_REGISTRY_ROOT HKLM
!define MEMENTO_REGISTRY_KEY ${UNINSTALL_KEY}

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_ICON ${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico
!define MUI_UNICON ${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico

!define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Header\orange.bmp
!define MUI_HEADERIMAGE_UNBITMAP ${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp

!define MUI_WELCOMEPAGE_TITLE "Welcome to the ZBar ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT \
    "This wizard will guide you through the installation of the \
    ZBar Bar Code Reader version ${VERSION}."

!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE "share\doc\zbar\LICENSE"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_COMPONENTSPAGE_CHECKBITMAP ${NSISDIR}\Contrib\Graphics\Checks\simple-round2.bmp

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

Function ShowREADME
     Exec '"notepad.exe" "$INSTDIR\README.windows"'
FunctionEnd

!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION ShowREADME
!define MUI_FINISHPAGE_LINK \
        "Visit the ZBar website for the latest news, FAQs and support"
!define MUI_FINISHPAGE_LINK_LOCATION "http://zbar.sourceforge.net/"

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "ZBar Core Files (required)" SecCore
    DetailPrint "Installing ZBar Program and Library..."
    SectionIn 1 2 RO

    SetOutPath $INSTDIR
    File share\doc\zbar\README.windows
    File share\doc\zbar\NEWS
    File share\doc\zbar\TODO
    File share\doc\zbar\COPYING
    File share\doc\zbar\LICENSE

    # emit a batch file to add the install directory to the path
    FileOpen $0 zbarvars.bat w
    FileWrite $0 "@rem  Add the ZBar installation directory to the path$\n"
    FileWrite $0 "@rem  so programs may be run from the command prompt$\n"
    FileWrite $0 "@set PATH=%PATH%;$INSTDIR\bin$\n"
    FileWrite $0 "@cd /D $INSTDIR$\n"
    FileWrite $0 "@echo For basic command instructions type:$\n"
    FileWrite $0 "@echo     zbarcam --help$\n"
    FileWrite $0 "@echo     zbarimg --help$\n"
    FileWrite $0 "@echo Try running:$\n"
    FileWrite $0 "@echo     zbarimg -d examples\barcode.png$\n"
    FileClose $0

    SetOutPath $INSTDIR\bin
    File bin\libzbar-0.dll
    File bin\zbarimg.exe
    File bin\zbarcam.exe

    # dependencies
    File ${PREFIX}\bin\zlib1.dll
    File ${PREFIX}\bin\libjpeg-7.dll
    File ${PREFIX}\bin\libpng12-0.dll
    File ${PREFIX}\bin\libtiff-3.dll
    File ${PREFIX}\bin\libxml2-2.dll
    File ${PREFIX}\bin\libiconv-2.dll
    File ${PREFIX}\bin\libMagickCore-2.dll
    File ${PREFIX}\bin\libMagickWand-2.dll

    FileOpen $0 zbarcam.bat w
    FileWrite $0 "@set PATH=%PATH%;$INSTDIR\bin$\n"
    FileWrite $0 "@echo This is the zbarcam output window.$\n"
    FileWrite $0 "@echo Hold a bar code in front of the camera (make sure it's in focus!)$\n"
    FileWrite $0 "@echo and decoded results will appear below.$\n"
    FileWrite $0 "@echo.$\n"
    FileWrite $0 "@echo Initializing camera, please wait...$\n"
    FileWrite $0 "@echo.$\n"
    FileWrite $0 "@zbarcam.exe --prescale=640x480$\n"
    FileWrite $0 "@if errorlevel 1 pause$\n"
    FileClose $0

    SetOutPath $INSTDIR\doc
    File share\doc\zbar\html\*

    SetOutPath $INSTDIR\examples
    File share\zbar\barcode.png
SectionEnd

#SectionGroup "Start Menu and Desktop Shortcuts" SecShortcuts
    Section "Start Menu Shortcuts" SecShortcutsStartMenu
        DetailPrint "Creating Start Menu Shortcuts..."
        SectionIn 1 2
        SetOutPath "${SMPROG_ZBAR}"
        #CreateShortCut "${SMPROG_ZBAR}\ZBar.lnk" "$INSTDIR\ZBar.exe"
        CreateDirectory "${SMPROG_ZBAR}"
        CreateShortCut "zbarcam.lnk" "$\"$INSTDIR\bin\zbarcam.bat$\"" "" \
                       "$INSTDIR\bin\zbarcam.exe"
        ExpandEnvStrings $0 '%comspec%'
        CreateShortCut "ZBar Command Prompt.lnk" \
                       $0 "/k $\"$\"$INSTDIR\zbarvars.bat$\"$\"" $0
        CreateShortCut "Command Reference.lnk" \
                       "$\"$INSTDIR\doc\ref.html$\""
    SectionEnd

#    Section "Desktop Shortcut" SecShortcutsDesktop
#        DetailPrint "Creating Desktop Shortcut..."
#        SectionIn 1 2
#        SetOutPath $INSTDIR
#        #CreateShortCut "$DESKTOP\ZBar.lnk" "$INSTDIR\ZBar.exe"
#    SectionEnd
#SectionGroupEnd

Section "Development Headers and Libraries" SecDevel
    DetailPrint "Installing ZBar Development Files..."
    SectionIn 2

    SetOutPath $INSTDIR\include
    File include\zbar.h

    SetOutPath $INSTDIR\include\zbar
    File include\zbar\Video.h
    File include\zbar\Exception.h
    File include\zbar\Symbol.h
    File include\zbar\Image.h
    File include\zbar\ImageScanner.h
    File include\zbar\Window.h
    File include\zbar\Processor.h
    File include\zbar\Decoder.h
    File include\zbar\Scanner.h

    SetOutPath $INSTDIR\lib
    File lib\libzbar-0.def
    File lib\libzbar-0.lib
    File lib\libzbar.dll.a

    SetOutPath $INSTDIR\examples
    File share\zbar\scan_image.cpp
    File share\zbar\scan_image.vcproj
SectionEnd

Section -post
    DetailPrint "Creating Registry Keys..."
    SetOutPath $INSTDIR
    WriteRegStr HKLM ${ZBAR_KEY} "InstallDir" $INSTDIR

    # register uninstaller
    WriteRegStr HKLM ${UNINSTALL_KEY} "UninstallString" \
                "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM ${UNINSTALL_KEY} "QuietUninstallString" \
                "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM ${UNINSTALL_KEY} "InstallLocation" "$\"$INSTDIR$\""

    WriteRegStr HKLM ${UNINSTALL_KEY} "DisplayName" "ZBar Bar Code Reader"
    WriteRegStr HKLM ${UNINSTALL_KEY} "DisplayIcon" "$INSTDIR\bin\zbarimg.exe,0"
    WriteRegStr HKLM ${UNINSTALL_KEY} "DisplayVersion" "${VERSION}"

    WriteRegStr HKLM ${UNINSTALL_KEY} "URLInfoAbout" "http://zbar.sf.net/"
    WriteRegStr HKLM ${UNINSTALL_KEY} "HelpLink" "http://zbar.sf.net/"
    WriteRegDWORD HKLM ${UNINSTALL_KEY} "NoModify" "1"
    WriteRegDWORD HKLM ${UNINSTALL_KEY} "NoRepair" "1"

    DetailPrint "Generating Uninstaller..."
    WriteUninstaller $INSTDIR\uninstall.exe
SectionEnd


Section Uninstall
    DetailPrint "Uninstalling ZBar Bar Code Reader.."

    DetailPrint "Deleting Files..."
    Delete $INSTDIR\examples\barcode.png
    Delete $INSTDIR\examples\scan_image.cpp
    Delete $INSTDIR\examples\scan_image.vcproj
    RMDir $INSTDIR\examples
    RMDir /r $INSTDIR\include
    RMDir /r $INSTDIR\doc
    RMDir /r $INSTDIR\lib
    RMDir /r $INSTDIR\bin
    Delete $INSTDIR\README.windows
    Delete $INSTDIR\NEWS
    Delete $INSTDIR\TODO
    Delete $INSTDIR\COPYING
    Delete $INSTDIR\LICENSE
    Delete $INSTDIR\zbarvars.bat
    Delete $INSTDIR\uninstall.exe
    RMDir $INSTDIR

    DetailPrint "Removing Shortcuts..."
    RMDir /r "${SMPROG_ZBAR}"

    DetailPrint "Deleting Registry Keys..."
    DeleteRegKey HKLM ${ZBAR_KEY}
    DeleteRegKey HKLM ${UNINSTALL_KEY}
SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} \
        "The core files required to use the bar code reader"
#    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} \
#        "Adds icons to your start menu and/or your desktop for easy access"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutsStartMenu} \
        "Adds shortcuts to your start menu"
#    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutsDesktop} \
#        "Adds an icon on your desktop"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDevel} \
        "Optional files used to develop other applications using ZBar"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
