; InstallerScript.nsi
;
; PolyBump(tm) Plugin Installer for 3DStudio MAX
;

; The name of the installer
Name "PolyBump(tm)"

; The file to write
OutFile "PolyBumpInstaller.exe"

; The default installation directory
InstallDir c:\3dsmax\plugins

; The text to prompt the user to enter a directory
DirText "Please choose the plugin folder of 3DStudio MAX"

;Installer Icon
;Icon "installer.ico"

LicenseText "End User License Agreement"
LicenseData "License.txt"

  
; The stuff to install
Section "ThisNameIsIgnoredSoWhyBother?" ;  ************************************************

; Set output path to the installation directory.
  SetOutPath $INSTDIR


  IfFileExists "$INSTDIR\..\3dsmax.exe" PathGood
	MessageBox MB_YESNO|MB_ICONQUESTION \
             "..\3DSMax.exe not found. Are you sure this is your 3DStudio Plugin folder?" \
             IDYES PathGood
      Quit
PathGood:


; Put file there
  File "D:\3dsmax6\plugins\PolyBumpPlugin.dlu"					; copy PolyBumpPlugin.dlu
  File "PolyBumpPlugin Product.pdf"								; copy documentation

SetOutPath $SYSDIR
IfFileExists "$SYSDIR\MSVCP70.DLL" BranchOver1					; copy MSVCP70.DLL
  File "C:windows\system32\MSVCP71.DLL"

BranchOver1:
IfFileExists "$SYSDIR\MSVCR70.DLL" BranchOver2					; copy MSVCR70.DLL
  File "C:windows\system32\MSVCR71.DLL"

BranchOver2:

; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PolyBumpMAXPlugin" "DisplayName" "PolyBumpMAXPlugin (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PolyBumpMAXPlugin" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "uninstall.exe"

; Shortcuts
  CreateDirectory "$SMPROGRAMS\Crytek\Polybump"
  CreateShortCut "$SMPROGRAMS\Crytek\Polybump\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Crytek\Polybump\Readme.lnk" "$INSTDIR\PolyBumpPlugin Product.pdf" "" "$INSTDIR\PolyBumpPlugin Product.pdf" 0


SectionEnd ; end the section **************************************************************






; uninstall stuff

UninstallText "This will uninstall PolyBump(tm). Hit next to continue."

; special uninstall section.
Section "Uninstall"
  ; remove registry keys
;  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PolyBumpMAXPlugin"
;  DeleteRegKey HKEY_CURRENT_USER "SOFTWARE\Crytek\PolyBumpMAXPlugin"
  ; remove files
  Delete $INSTDIR\PolyBumpPlugin.dlu
  Delete "$INSTDIR\PolyBump Plugin for 3DStudio MAX.pdf"
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\Crytek\Polybump\*.*"
  ; remove directories used.
  RMDir "$SMPROGRAMS\Crytek\Polybump"
  RMDir "$SMPROGRAMS\Crytek"
SectionEnd




; eof
