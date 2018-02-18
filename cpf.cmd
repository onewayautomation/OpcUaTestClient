robocopy %1 %2 %3 %4 %5
IF %ERRORLEVEL% LSS 8 goto finish
echo some file(s) could not be copied & GOTO :eof
:finish
EXIT 0