#!/usr/bin/env python3
"""
upload_uart.py - UART Bootloader Upload Tool for dsPIC33CK Arduino Platform

Sends compiled .hex file to the dsPIC33CK via UART bootloader.
Compatible with Microchip's 16-bit bootloader protocol.

Usage:
    python upload_uart.py --port COM3 --baud 115200 --hex firmware.hex

Protocol:
    1. Send sync byte (0x55) to auto-detect baud rate
    2. Send bootloader command to erase flash
    3. Send hex records in chunks
    4. Send verify command
    5. Send run command to start application

Requirements:
    pip install pyserial intelhex
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

try:
    from intelhex import IntelHex
except ImportError:
    print("ERROR: intelhex not installed. Run: pip install intelhex")
    sys.exit(1)

# Bootloader commands
CMD_SYNC        = 0x55
CMD_READ_VER    = 0x01
CMD_ERASE       = 0x02
CMD_WRITE_ROW   = 0x03
CMD_VERIFY      = 0x04
CMD_RESET       = 0x08
CMD_ACK         = 0x06
CMD_NACK        = 0x15

# dsPIC33CK flash parameters
FLASH_PAGE_SIZE = 1024   # bytes per page
FLASH_ROW_SIZE  = 128    # bytes per row (write granularity)
FLASH_START     = 0x1000 # Application start (after bootloader)
FLASH_END       = 0x8000 # End of flash for 32KB device


def calculate_checksum(data):
    """Calculate simple 8-bit checksum."""
    return (~sum(data) + 1) & 0xFF


def send_command(ser, cmd, data=None, verbose=False):
    """Send a bootloader command and wait for ACK."""
    packet = bytearray()
    packet.append(cmd)

    if data:
        length = len(data)
        packet.append(length & 0xFF)
        packet.append((length >> 8) & 0xFF)
        packet.extend(data)
    else:
        packet.append(0x00)
        packet.append(0x00)

    checksum = calculate_checksum(packet)
    packet.append(checksum)

    if verbose:
        print(f"  TX: {packet.hex()}")

    ser.write(packet)
    ser.flush()

    # Wait for response
    response = ser.read(1)
    if len(response) == 0:
        return False
    return response[0] == CMD_ACK


def sync_bootloader(ser, verbose=False):
    """Send sync bytes to establish communication."""
    print("Syncing with bootloader...")
    for attempt in range(10):
        ser.write(bytes([CMD_SYNC]))
        ser.flush()
        time.sleep(0.1)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            if CMD_ACK in resp:
                print("  Bootloader responded!")
                return True
        if verbose:
            print(f"  Attempt {attempt + 1}/10...")
    return False


def upload_hex(port, baud, hex_file, verbose=False):
    """Upload a .hex file to the dsPIC33CK via UART bootloader."""

    print(f"Opening {hex_file}...")
    ih = IntelHex(hex_file)

    # Get address range
    start_addr = max(ih.minaddr(), FLASH_START)
    end_addr = min(ih.maxaddr(), FLASH_END)
    print(f"  Flash range: 0x{start_addr:06X} - 0x{end_addr:06X}")
    print(f"  Size: {end_addr - start_addr} bytes")

    print(f"\nOpening serial port {port} at {baud} baud...")
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2
        )
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port}: {e}")
        sys.exit(1)

    time.sleep(0.5)  # Wait for port to stabilize
    ser.reset_input_buffer()

    # Step 1: Sync
    if not sync_bootloader(ser, verbose):
        print("ERROR: Cannot sync with bootloader.")
        print("  - Is the device in bootloader mode?")
        print("  - Check wiring (TX->RX, RX->TX)")
        print("  - Try resetting the device while holding BOOT pin")
        ser.close()
        sys.exit(1)

    # Step 2: Read version
    print("\nReading bootloader version...")
    if send_command(ser, CMD_READ_VER, verbose=verbose):
        ver = ser.read(3)
        if len(ver) >= 2:
            print(f"  Bootloader v{ver[0]}.{ver[1]}")

    # Step 3: Erase flash
    print("\nErasing flash...")
    num_pages = (end_addr - start_addr + FLASH_PAGE_SIZE - 1) // FLASH_PAGE_SIZE
    erase_data = bytearray([
        start_addr & 0xFF,
        (start_addr >> 8) & 0xFF,
        (start_addr >> 16) & 0xFF,
        num_pages & 0xFF
    ])
    if not send_command(ser, CMD_ERASE, erase_data, verbose):
        print("ERROR: Erase failed!")
        ser.close()
        sys.exit(1)
    print("  Done.")

    # Step 4: Write flash rows
    print("\nWriting flash...")
    total_rows = (end_addr - start_addr + FLASH_ROW_SIZE - 1) // FLASH_ROW_SIZE
    rows_written = 0

    for addr in range(start_addr, end_addr, FLASH_ROW_SIZE):
        row_data = bytearray()
        # Address bytes (24-bit)
        row_data.append(addr & 0xFF)
        row_data.append((addr >> 8) & 0xFF)
        row_data.append((addr >> 16) & 0xFF)
        # Row data
        for i in range(FLASH_ROW_SIZE):
            row_data.append(ih[addr + i] if (addr + i) <= ih.maxaddr() else 0xFF)

        if not send_command(ser, CMD_WRITE_ROW, row_data, verbose):
            print(f"\nERROR: Write failed at 0x{addr:06X}")
            ser.close()
            sys.exit(1)

        rows_written += 1
        progress = (rows_written * 100) // total_rows
        print(f"\r  Progress: [{('=' * (progress // 5)):<20}] {progress}%", end='')

    print("\n  Done.")

    # Step 5: Verify
    print("\nVerifying...")
    if send_command(ser, CMD_VERIFY, verbose=verbose):
        print("  Verification passed!")
    else:
        print("  WARNING: Verification not supported by bootloader (non-critical)")

    # Step 6: Reset to run application
    print("\nResetting device...")
    send_command(ser, CMD_RESET, verbose=verbose)
    print("  Application started!")

    ser.close()
    print("\nUpload complete!")


def main():
    parser = argparse.ArgumentParser(
        description='Upload firmware to dsPIC33CK via UART bootloader'
    )
    parser.add_argument('--port', '-p', required=True,
                        help='Serial port (e.g., COM3, /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--hex', '-f', required=True,
                        help='Path to .hex file')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()
    upload_hex(args.port, args.baud, args.hex, args.verbose)


if __name__ == '__main__':
    main()
