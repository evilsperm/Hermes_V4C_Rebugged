set PS3DEV2=i:/ps3dev
set MSYS=i:/ps3dev/msys
set PATH=%PS3DEV%/bin;%PS3DEV2%/ppu/bin;%PS3DEV2%/spu/bin;%PS3DEV2%/MinGW/bin;%PS3DEV2%/msys/1.0/bin;%PATH%;
set PS3_COMPILERS= i:/ps3dev/ppu/bin
make clean
make
copy payload_groove_hermes.bin ..\data\payload_groove_hermes.bin
make clean
pause
