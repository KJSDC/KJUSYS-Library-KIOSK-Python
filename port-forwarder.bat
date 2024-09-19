@echo off

echo Port forwarding localhost:80
@REM command to start the ngrok port forwarding
ngrok.exe http --domain=bulldog-promoted-accurately.ngrok-free.app http://127.0.0.1:80/

pause
