@echo off
setlocal
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64
makeappx pack /d %1 /p %2.msix