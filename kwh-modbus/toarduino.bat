echo off
for /f "tokens=1,2*" %%A in ('reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal" 2^>nul') do set docsdir=%%C

set arduinodir=%docsdir%\Arduino\libraries
set arduinodir=%arduinodir:\=/%
set arduinodir=%arduinodir:C:/=/mnt/c/%

bash -c "./toarduino.sh %arduinodir%"