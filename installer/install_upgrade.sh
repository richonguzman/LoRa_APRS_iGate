#!/bin/bash
echo "Your firmware will be upgraded without factory reset."
echo "IF THIS IS YOUR FIRST FLASH ON THIS BOARD PLEASE USE install_with_factory_reset.sh INSTEAD!"

read -p "Enter COM port (for example /dev/ttyS5): " port

python3 ./bin/esptool/esptool.py --chip esp32 --port "$port" --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 firmware/bootloader.bin 0x8000 firmware/partitions.bin 0xe000 firmware/boot_app0.bin 0x10000 firmware/firmware.bin

echo "Firmware flashed"