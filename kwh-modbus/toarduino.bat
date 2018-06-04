echo off
for /f "tokens=1,2*" %%A in ('reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal" 2^>nul') do set docsdir=%%C

set word=/mnt/c/
call set docsdir=%%docsdir:"C:\"=%word%%%
echo %docsdir%

set arduinodir=%docsdir%\Arduino\libraries

bash -c "./toarduino.sh %arduinodir%"