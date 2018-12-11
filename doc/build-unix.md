## Alphacond build instructions for \*nix

### Dependency installation for Ubuntu

Build requirements:

	sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils libdb++-dev python3 libboost-all-dev libminiupnpc-dev

BerkeleyDB is required for the wallet:

	sudo apt-get install software-properties-common
	sudo add-apt-repository ppa:bitcoin/bitcoin
	sudo apt-get update
	sudo apt-get install libdb4.8-dev libdb4.8++-dev

### Building

To build run this commands:
	
	cd src/
    make -f makefile.unix
