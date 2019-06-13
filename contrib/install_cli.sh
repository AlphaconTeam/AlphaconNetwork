 #!/usr/bin/env bash

 # Execute this file to install the alphacon cli tools into your path on OS X

 CURRENT_LOC="$( cd "$(dirname "$0")" ; pwd -P )"
 LOCATION=${CURRENT_LOC%Alphacon-Qt.app*}

 # Ensure that the directory to symlink to exists
 sudo mkdir -p /usr/local/bin

 # Create symlinks to the cli tools
 sudo ln -s ${LOCATION}/Alphacon-Qt.app/Contents/MacOS/alphacond /usr/local/bin/alphacond
 sudo ln -s ${LOCATION}/Alphacon-Qt.app/Contents/MacOS/alphacon-cli /usr/local/bin/alphacon-cli
