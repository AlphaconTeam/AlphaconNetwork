
Debian
====================
This directory contains files used to package alphacond/alphacon-qt
for Debian-based Linux systems. If you compile alphacond/alphacon-qt yourself, there are some useful files here.

## alphacon: URI support ##


alphacon-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install alphacon-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your alphacon-qt binary to `/usr/bin`
and the `../../share/pixmaps/alphacon128.png` to `/usr/share/pixmaps`

alphacon-qt.protocol (KDE)

