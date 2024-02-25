@echo off
echo Your current configuration will be LOST!!!
echo Your current configuration will be LOST!!!
echo Your current configuration will be LOST!!!
echo If you already have this board flashed with our firmware please use install_upgrade.bat instead!

set /p port="Enter COM port (for example COM5): "

python.exe bin/esptool/esptool.py --chip esp32 --port "%port%" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 2686976 firmware/spiffs.bin

python.exe bin/esptool/esptool.py --chip esp32 --port "%port%" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 firmware/bootloader.bin 0x8000 firmware/partitions.bin 0xe000 firmware/boot_app0.bin 0x10000 firmware/firmware.bin

echo "Firmware flashed"
pause