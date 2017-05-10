# ![Otter Browser logo](resources/icons/otter-browser-64.png) Otter Browser


***A browser controlled by the user, not vice-versa***

[**Otter Browser**](https://otter-browser.org/) aims to recreate the best aspects of **Opera 12** and to revive its spirit. We are working on providing the powerful features power users want while keeping the browser fast and lightweight. We also learned from History and decided to release the browser under the GNU GPL v3.

[![SourceForge](https://img.shields.io/sourceforge/dt/otter-browser.svg)](https://sourceforge.net/projects/otter-browser/files/)

[![Screenshot](https://otter-browser.org/screenshots/1.png)](https://otter-browser.org/screenshots/)

Planned features are listed in the *TODO* file. Details on already implemented features are available in the *CHANGELOG* file. If you have an idea that has not yet been proposed or refused, feel free to [open a new issue](https://github.com/OtterBrowser/otter-browser/issues/new).

## Install

You can either compile Otter Browser from source or use pre-compiled binaries.

### From source

To build Otter, you will need the following depedencies: Qt (at least 5.4.0), Sonnet, and CMake (3.1.0 or newer). At the root of the directory were the source code is stored, execute these commands:

    mkdir build
    cd build
    cmake ../
    make
    make install

Detailed instructions are available in the *INSTALL* file at the root of the repository.

### Under Linux and *BSD

Linux users can use the official AppImage version available at [SourceForge](https://sourceforge.net/projects/otter-browser/files/). It is a single executable file that doesn’t need any dependencies to be installed. The AppImage version should run under any system installed after 2012. The browser is also available in a the repositories of a wide range of Linux distributions and *BSD systems. Read more on [the dedicated wiki page](https://github.com/OtterBrowser/otter-browser/wiki/Packages).

### Under Windows

Windows users can download binary releases from [SourceForge](https://sourceforge.net/projects/otter-browser/files/).

### Under macOS

DMG packages are available at [SourceForge](https://sourceforge.net/projects/otter-browser/files/).

## How to contribute

Otter Browser is *your* browser. Because it is free software (GPL v3), you can contribute to make it better. New contributors are always welcome, whether you write code, make resources, report bugs, or suggest features.

The browser is written in C++ from scratch and is based on the Qt5 framework, with some modules written in JavaScript. Have a look on [open issues](https://github.com/OtterBrowser/otter-browser/issues) to find a mission that suits you.

We use [Transifex](https://www.transifex.com/otter-browser/otter-browser/) to translate Otter Browser.

Too stay tuned on the development, you can join [the official forum](http://thedndsanctuary.eu/index.php?board=9.0) and two IRC channels at Freenode: [#otter-browser](http://irc.lc/freenode/otter-browser) (main and international) and [#otter-browser-pl](http://irc.lc/freenode/otter-browser-pl) (Polski / Polish).

Read *CONTRIBUTING.md* and don’t hesitate!

