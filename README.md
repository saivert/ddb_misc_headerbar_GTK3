# ddb_misc_headerbar_GTK3
Adds a headerbar to your DeaDBeeF.

[![Build Status](https://drone.io/github.com/saivert/ddb_misc_headerbar_GTK3/status.png)](https://drone.io/github.com/saivert/ddb_misc_headerbar_GTK3/latest)

Requirements
------------
* Version 0.7 or higher of DeaDBeeF.
* GTK3

Screenshot
----------
![Screenshot](http://i.imgur.com/zYXzoSp.png "Screenshot")

Installation
------------

Manually from source:

    $ git clone https://github.com/saivert/ddb_misc_headerbar_GTK3.git
    $ cd ddb_misc_headerbar_GTK3
    $ ./autogen.sh
    $ ./configure && make
    The last step copies the plugin to ~/.local/lib64/deadbeef
    $ cd src && make localinstall
    If you have a 32 bit system copy it yourself to the right place, e.g:
    $ cp src/.libs/ddb_misc_headerbar_GTK3.so ~/.local/lib/deadbeef

For Arch linux you can use the PKGBUILD in the distro/arch directory.

Binary builds
-------------

Static builds are not ready yet, I'm waiting on deadbeef-plugin-builder to be updated.

Build scripts for your favorite distro are accepted :)
