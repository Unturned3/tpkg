tpkg: tiny package manager for MicroLinux
=========================================

WARNING: tpkg is *highly experimental*, and it might cause
         damage to your system/data. Use at your own risk.

How to use:
===========

1. Run "mkdir -p /path/to/root/usr/bin/tpkg_DB"
   This creates a fake root directory for tpkg to operate on.
   /path/to/root/usr/bin: this is where all the downloaded
   binaries will be installed to
   /path/to/root/usr/bin/tpkg_DB: this is the tpkg package
   database
2. sudo chown root:root -R /path/to/root
3. sudo chmod 755 -R /path/to/root
4. Edit tpkg.c with a text editor, and change the ROOT macro
   to point to the /path/to/root directory you made. This must
   be a full path, *NOT* a relative path.
