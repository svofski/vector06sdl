SET /P GODOT_VERSION=<GODOT-VERSION
echo "v06x-godot version: %X%"

del /q/s dist\%GODOT_VERSION%
mkdir dist\%GODOT_VERSION%

del /q/s exports\win64
mkdir exports\win64
del /q/s exports\linux
mkdir exports\linux
set GODOT_BIN=%USERPROFILE%\bin\Godot\Godot_v3.5-stable_win64.exe 

cd v06x
%GODOT_BIN% --no-window v06x/project.godot --export "Linux/X11"
%GODOT_BIN% --no-window v06x/project.godot --export "Windows Desktop"

cd ..\exports\win64
set WIN64_DISTRO="v06x-%GODOT_VERSION%-win64.zip"
echo "Building %WIN64_DISTRO%"
mkdir boot
copy ..\..\..\..\boot\* boot\
zip -r %WIN64_DISTRO% .
zip %WIN64_DISTRO% -z<..\..\zipfile-comment.txt
move %WIN64_DISTRO% ..\..\dist\%GODOT_VERSION%\


set LINUX_DISTRO="v06x-%GODOT_VERSION%-linux_x64.zip"
echo "Building %LINUX_DISTRO%"
cd ..\linux
mkdir boot
copy ..\..\..\..\boot\* boot\
zip -r %LINUX_DISTRO% .
zip %LINUX_DISTRO% -z<..\..\zipfile-comment.txt
move %LINUX_DISTRO% ..\..\dist\%GODOT_VERSION%\

