# ![Otter Browser logo](resources/icons/otter-browser-64.png) Otter Browser


***A browser controlled by the user, not vice-versa***

[**Otter Browser**](https://otter-browser.org/) aims to recreate the best aspects of **Opera 12** and to revive its spirit. We are focused on providing the powerful features power users want while keeping the browser fast and lightweight. We also learned from history and decided to release the browser under the GNU GPL v3.

[![SourceForge](https://img.shields.io/sourceforge/dt/otter-browser.svg)](https://sourceforge.net/projects/otter-browser/files/)

[![Screenshot](https://otter-browser.org/screenshots/1.png)](https://otter-browser.org/screenshots/)

Planned features are listed in the *TODO* file. Details on already implemented features are available in the *CHANGELOG* file. If you have an idea that has not yet been proposed or rejected, feel free to [open a new issue](https://github.com/OtterBrowser/otter-browser/issues/new).

## Install

You can either compile Otter Browser from source or use pre-compiled binaries.

### From source

To build Otter Browser, you will need the following dependencies: **Qt 5.4.0** (or newer), **OpenSSL 1.0** (or newer, but not 1.1), **GStreamer 1.0** (or newer) and codecs, and **CMake 3.1.0** (or newer). At the root of the directory were the source code is stored, execute these commands:

    mkdir build
    cd build
    cmake ../
    make
    make install

Detailed instructions are available in the *INSTALL.md* file in the root of the repository.

### Under Linux and *BSD

[![Arch Linux logo](https://github.com/m4sk1n/logos/raw/master/32x32/archlinux.png)](https://u.m4sk.in/arch)
[![CentOS logo](https://github.com/m4sk1n/logos/raw/master/32x32/centos.png)](https://u.m4sk.in/centos)
[![Fedora logo](https://github.com/m4sk1n/logos/raw/master/32x32/fedora.png)](https://u.m4sk.in/fedora)
[![FreeBSD logo](https://github.com/m4sk1n/logos/raw/master/32x32/freebsd.png)](https://u.m4sk.in/freebsd)
[![Gentoo logo](https://github.com/m4sk1n/logos/raw/master/32x32/gentoo.png)](https://u.m4sk.in/gentoo)
[![openSUSE logo](https://github.com/m4sk1n/logos/raw/master/32x32/opensuse.png)](https://u.m4sk.in/suse)
[![Sparky Linux logo](https://github.com/m4sk1n/logos/raw/master/32x32/sparky.png)](https://u.m4sk.in/sparky)
[![Ubuntu logo](https://github.com/m4sk1n/logos/raw/master/32x32/ubuntu.png)](https://u.m4sk.in/ubuntu)
[![Void Linux logo](https://github.com/m4sk1n/logos/raw/master/32x32/void.png)](https://u.m4sk.in/void)
[![Tux](https://github.com/m4sk1n/logos/raw/master/32x32/tux.png)](https://u.m4sk.in/appimage)

Linux users can use the official AppImage version available on [SourceForge](https://sourceforge.net/projects/otter-browser/files/). It is a single executable file that doesn’t need any dependencies to be installed. The AppImage version should run under any system installed after 2012 provided it has OpenSSL 1.0.x (not 1.1.x) and GStreamer 1.x (with codecs). The browser is also available in the repositories of a wide range of Linux distributions and *BSD systems. Read more on [the dedicated wiki page](https://github.com/OtterBrowser/otter-browser/wiki/Packages).

### Under Windows

Windows users can download binary releases on [SourceForge](https://sourceforge.net/projects/otter-browser/files/).

### Under macOS

DMG packages are available on [SourceForge](https://sourceforge.net/projects/otter-browser/files/).

## How to contribute

Otter Browser is *your* browser. Because it is free software (GPL v3), you can contribute to make it better. New contributors are always welcome, whether you write code, create resources, report bugs, or suggest features.

The browser is written primarily in C++ and leverages powerful features offered by the Qt5 framework.

We also use JavaScript for interacting with rendering engines (when native APIs are not available) and Python 3 is our preferred language for creating tools to ease development.

Have a look at the [open issues](https://github.com/OtterBrowser/otter-browser/issues) to find a mission that resonates with you.

We use [Transifex](https://www.transifex.com/otter-browser/otter-browser/) to translate Otter Browser.

To stay informed about Otter development, bug fixes and new features, you can join [the official forum](http://thedndsanctuary.eu/index.php?board=9.0). We also have two IRC channels on Freenode: [#otter-browser](http://irc.lc/freenode/otter-browser) (international) and [#otter-browser-pl](http://irc.lc/freenode/otter-browser-pl) (polski / Polish).

Read *CONTRIBUTING.md* and don’t hesitate!

