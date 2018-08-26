tpkg: tiny package manager for MicroLinux
=========================================

**WARNING**: tpkg is *highly experimental*, and it might cause
             damage to your system/data. Use at your own risk.

Online package repository URL: https://raw.githubusercontent.com/Unturned3/unturned3.github.io/master/repo


### How to Install:

1. Run "mkdir -p /path/to/root/usr/bin/tpkg_DB".
   This creates a fake root directory for tpkg to operate on,
   so it won't mess up your actual /usr/bin.
   "/path/to/root/usr/bin" is where all the downloaded
   binaries will be installed to, and
   "/path/to/root/usr/bin/tpkg_DB" is the tpkg package
   database
2. sudo chown root:root -R /path/to/root
3. sudo chmod 755 -R /path/to/root
4. Edit tpkg.c with a text editor, and change the ROOT macro
   to point to the "/path/to/root" directory you made. This must
   be a full path, *NOT* a relative path.
5. Run "make" and a binary named "tpkg" will be generated

### How to Use:
1. tpkg needs to run as root, so invoke it by "sudo ./tpkg [options]"
2. tpkg has two options: -i to install packages, and -r to remove installed packages
3. Example: "sudo ./tpkg -i foo bar". This will install the packages named "foo" and "bar"

### What packages are available?
Currently, four packages are availabe, and they are named "a", "b", "c", and "d".
These are just simple shell scripts that prints text to the terminal.
More packages will be added later, once tpkg becomes usable.
