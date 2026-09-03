OpenRemote Studio 2.70 for Linux
================================

    OpenRemote Studio 2.70 x86_64.AppImage

That single file is the whole program. It is the Linux equivalent of a .app on
Mac or a .exe on Windows. There is no installer and nothing to set up.


HOW TO RUN IT
-------------
Linux will not run a downloaded file until you give it permission. This is a
one-time step:

    1. Right-click the file and choose Properties.
    2. Go to the Permissions tab.
    3. Tick "Allow executing file as program".
       (On KDE this is "Is executable"; on some desktops it sits under a
       Details or Execute heading. Same setting, different wording.)
    4. Close Properties and double-click the file.

Studio opens in your web browser. No terminal window appears.

If you would rather use a terminal, this does exactly the same thing:

    chmod +x "OpenRemote Studio 2.70 x86_64.AppImage"
    ./"OpenRemote Studio 2.70 x86_64.AppImage"

Every AppImage from every vendor works this way. It is a Linux security rule,
not something specific to this program.


WHAT IF DOUBLE-CLICKING DOES NOTHING?
-------------------------------------
Almost always it is the executable permission above not being set. Check that
first.

Some file managers open unknown files in a text editor instead of running
them. If that happens, right-click the file and look for "Run", "Execute", or
"Open With -> Run Software Install" in the menu.

Give it a few seconds on first launch. It has to unpack itself the first time.


PUTTING IT IN YOUR APPLICATIONS MENU
------------------------------------
Optional. The easiest way is a small tool called Gear Lever, available in most
software centres or from Flathub. Open the AppImage with Gear Lever and it
creates the menu entry and icon for you, and keeps the file tidy.

Without it, the AppImage still runs perfectly well from wherever you saved it.


REQUIREMENTS
------------
A 64-bit Intel or AMD Linux system (x86_64, sometimes called amd64) and a web
browser. That is everything.

Python is inside the file. You do not need to install Python, pip, Arduino IDE,
PlatformIO, or have an internet connection.

This build will NOT run on ARM machines such as a Raspberry Pi. Ask if you need
an aarch64 build.


CONNECTING THE REMOTE OVER USB
------------------------------
Plug the remote in with a USB cable. It appears as /dev/ttyACM0, or
/dev/ttyUSB0 if it is behind a USB-to-serial adapter.

If Studio says it cannot open the port, your user account is probably not in
the group that owns serial devices. Add yourself to it:

    sudo usermod -aG dialout $USER      Debian, Ubuntu, Mint
    sudo usermod -aG uucp $USER         Arch, Fedora

Then log out and back in, or reboot. The change does not take effect until you
do.

Studio reads the actual owning group from the device and names it in the error
message, so if your distribution uses a different name, follow what Studio
tells you rather than guessing from the two above.


FILE CHOOSER DIALOGS
--------------------
"Set Up a New Remote" needs to open a file picker. Linux has no single built-in
dialog for this, so Studio uses whichever one your desktop provides:

    zenity     GNOME, Cinnamon, XFCE and most GTK desktops
    kdialog    KDE Plasma

One of these is almost always installed already. If neither is, Studio tells
you which to install. Everything that does not involve picking a file works
regardless.


A NOTE ON THE WINDOW
--------------------
On Mac and Windows, Studio appears in its own application window. Linux has no
built-in web engine that is guaranteed to be present on every system, so Studio
opens in your normal browser instead. Only the window frame differs - the
program and all of its features are exactly the same.


UNINSTALLING
------------
Delete the AppImage file.

Settings and the IR database are stored in these two folders, which you can
also delete if you want every trace gone:

    ~/.local/share/OpenRemoteStudio
    ~/.cache/OpenRemoteStudio


SETTING UP A NEW REMOTE
-----------------------
Remote Config includes Set Up a New Remote. Choose any compatible OpenRemote
firmware .bin and WebConfig .html file. Studio shows the versions it detects
from those files, verifies the ESP32-S3, and installs the firmware.

For SD card setup, put the FAT32 card in your computer, not the remote, then
choose the card in Studio. Studio creates all the folders it needs and installs
the selected WebConfig as www/index.html.

The remote programs itself automatically over USB. It has only a Reset button;
there is no BOOT button to hold down.

After Detect succeeds, Install Sensor Test becomes available. It installs the
bundled Rev 6 hardware self-test firmware. It checks every display bus
pin, the microSD card and the microphone, and prints a pass/fail report to
the USB serial monitor without touching the SD card. When
you have finished testing, use Install to put the normal OpenRemote firmware
back. IRDB Browser sends selected .ir devices to the SD card only while that
card is in the remote.


PLEASE READ - TESTING STATUS
----------------------------
This is the first Linux release and it has not yet been run on an actual Linux
machine. It was built and checked carefully on a Mac, and its contents were
verified in detail, but verifying a file is not the same as running it.

USB flashing and SD card setup in particular are untested on Linux.

If something does not work, please report it rather than assuming you have done
something wrong.
