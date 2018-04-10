[![Build Status](https://travis-ci.org/tekezo/Karabiner-Elements.svg?branch=master)](https://travis-ci.org/tekezo/Karabiner-Elements)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/tekezo/Karabiner-Elements/blob/master/LICENSE.md)

# Karabiner-Elements

Karabiner-Elements is a powerful utility for keyboard customization on macOS Sierra or later.

[Karabiner](https://pqrs.org/osx/karabiner) stopped working because of the keyboard driver architecture changes at macOS Sierra.
Thus, Karabiner-Elements was made from scratch for new macOS.

## Karabiner build
* 22 oktober 2016: [Karabiner-Elements-0.90.37.dmg](https://raw.githubusercontent.com/erikdejonge/Karabiner-Elements/master/Karabiner-Elements-0.90.37.dmg)


## Project Status

Karabiner-Elements is ready to use today. It provides a useful subset of Karabiner's features that you can begin using immediately.

You can download the latest Karabiner-Elements from https://pqrs.org/osx/karabiner/

### Old releases

You can download previous versions of Karabiner-Elements from here:
https://github.com/tekezo/pqrs.org/tree/master/webroot/osx/karabiner/files

## System requirements

* macOS 10.12.*
* macOS 10.13.*

# Usage

https://pqrs.org/osx/karabiner/document.html

## How to build

System requirements:

* macOS 10.13+
* Xcode 9+
* Command Line Tools for Xcode
* Boost 1.61.0+ (header-only) http://www.boost.org/

To install the Boost requirement, [download the latest Boost release](http://www.boost.org/), open the `boost` folder inside of it, and move all of the files there into `/opt/local/include/boost/`.

(For example, the version.hpp should be located in `/opt/local/include/boost/version.hpp`)


### Step 1: Getting source code

Clone the source from github.

``` bash
git clone --depth 1 https://github.com/tekezo/Karabiner-Elements.git
```

### Step 2: Building a package

``` bash
cd Karabiner-Elements
make
```

The **make** script will create a redistributable **Karabiner-Elements-VERSION.dmg** in the current directory.

# Donations

If you would like to contribute financially to the development of Karabiner Elements, donations can be made via https://pqrs.org/osx/karabiner/pricing.html
