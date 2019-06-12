# Dj Queue ESP32 Source Code

## Jackson Wheeler
### Using the ESP ADF Library



## Compatibility
You need to be using the ESP-32 Lyrat board.
And using the ESP-ADF library.
ESP32 Espressif Arduino library wont work!
Neither will a normal ESP-32.

## Usage

Install the espressif ESP-ADF library

Configure the installation with `make menuconfig`:

- Set up the Wi-Fi and BT connection in `menuconfig` -> `Example Configuration` -> Fill in `Wi-Fi SSID` & `WiFi Password` and `BT remote device name`.
- You also need to set the device flashing port used, the will usually be "/dev/cu.SLAB_USBtoUART"


Flash the board and setup a BT Speaker:

- Set the name of the bluetooth speaker you will use.
- Switch the BT Speaker on.

Flash and run djqueue:

- you will need to hit the pair button on the bt speaker after the board is running.
- If all is working, the speaker will say 'DJ Queue is ready to play'

