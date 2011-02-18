set PS3LOAD=tcp:192.168.2.12

@set PS3DEV=i:/ps3dev

@set PATH=%PS3DEV%/cygwin;%PATH%;
ps3load.exe *.self
pause