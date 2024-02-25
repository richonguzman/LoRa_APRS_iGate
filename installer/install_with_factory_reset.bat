@echo off
echo Loading...

where /q python.exe
if %errorlevel% neq 0 (
    echo "Python is not installed. Please install it and try again."
    exit /b
)

where /q pip
if %errorlevel% neq 0 (
    echo "pip is not installed. Please install it and try again."
    exit /b
)

pip show pyserial >nul 2>&1
if %errorlevel% neq 0 (
    echo "Installing pyserial..."
    pip install pyserial
)


echo Your current configuration will be LOST!!!
echo Your current configuration will be LOST!!!
echo Your current configuration will be LOST!!!

echo:

echo If you already have this board flashed with our firmware please use install_upgrade.bat instead!

echo:

echo Available COM ports:
python.exe -c "import serial.tools.list_ports; print('\n'.join([str(c) for c in serial.tools.list_ports.comports()]))"

echo:

set /p port="Enter COM port (for example COM5): "

python.exe bin/esptool/esptool.py --chip esp32 --port "%port%" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 2686976 firmware/spiffs.bin

python.exe bin/esptool/esptool.py --chip esp32 --port "%port%" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 firmware/bootloader.bin 0x8000 firmware/partitions.bin 0xe000 firmware/boot_app0.bin 0x10000 firmware/firmware.bin

pause