@echo off
setlocal
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64
signtool sign /p "Password12$" /a /v /fd SHA256 /f "bin\release\PSFScriptingSample_TemporaryKey.pfx" %1 
