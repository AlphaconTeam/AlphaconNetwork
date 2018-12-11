## Alphacond build instructions for macOS

### Preparation

You need to install XCode with all the options checked so that the compiler
and everything is available in /usr not just /Developer. XCode should be
available on your OS X installation media, but if not, you can get the
current version from https://developer.apple.com/xcode/. If you install
Xcode 4.3 or later, you'll need to install its command line tools. This can
be done in `Xcode > Preferences > Downloads > Components` and generally must
be re-done or updated every time Xcode is updated.

You will also need to install [Homebrew](https://brew.sh). 

### Install dependencies

	brew install boost miniupnpc openssl berkeley-db4

Note: After you have installed the dependencies, you should check that the Brew installed version of OpenSSL is the one available for compilation. You can check this by typing

	openssl version

into Terminal. You should see OpenSSL 1.0.1e 11 Feb 2013.

If not, you can ensure that the Brew OpenSSL is correctly linked by running

	brew link openssl --force

Rerunning "openssl version" should now return the correct version.

### Building

To build run this commands:
	
	cd src/
    make -f makefile.mac
