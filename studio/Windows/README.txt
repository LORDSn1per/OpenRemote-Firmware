OpenRemote Studio 2.70 for Windows
==================================

1. Extract the complete OpenRemoteStudio_v2_45_Windows folder.
2. Double-click OpenRemote Studio 2.70.exe.
3. OpenRemote Studio will open in your default web browser.

Python 3.12 and USB serial support are included. You do not need to install
Python. Keep the runtime and app folders beside OpenRemote Studio.exe.

Closing the browser tab also closes the hidden Studio server after a short
idle period. Reopening OpenRemote Studio.exe will start or reuse the correct
session without leaving duplicate background processes.

Supported systems: 64-bit Windows 10 and Windows 11.

NEW REMOTE SETUP
----------------
Remote Config includes Set Up a New Remote. Choose any compatible OpenRemote
firmware .bin and WebConfig .html file. Studio shows the versions detected from
those files, verifies the ESP32-S3, and installs the firmware. For SD card
setup, insert the FAT32 card into the computer, not the remote, then choose the
card in Studio. Studio creates all required folders and installs the selected
WebConfig as www/index.html. The remote uses automatic USB programming and has
only a Reset button; there is no BOOT button to hold.
Arduino IDE, PlatformIO, Python, and internet access are not required.

After Detect succeeds, Install Sensor Test becomes available. It installs the
bundled Rev 6 hardware self-test firmware. It checks every display bus
pin, the microSD card and the microphone, and prints a pass/fail report to
the USB serial monitor without changing the SD card. When
testing is complete, use Install to restore the OpenRemote firmware selected in
the firmware chooser. Only IRDB Browser sends selected .ir devices to the SD
card while it is installed in the remote.
