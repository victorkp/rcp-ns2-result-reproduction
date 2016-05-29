#!/bin/bash

set -e

echo
echo "Installing dependencies..."
echo "[>               ]"
sudo apt-get update > /dev/null
echo "[==>             ]"

# When reprovisioning, install "fails" because there's nothing to do
set +e
echo "[====>           ]   Installing build tools"
sudo apt-get install build-essential autoconf automake libxmu-dev gcc-4.4  -y > /dev/null 2>&1
echo "[========>       ]"
sudo apt-get install automake libxmu-dev gcc-4.4  -y > /dev/null 2>&1
echo "[============>   ]   Installing Perl and x11 packages"
sudo apt-get install perl gnuplot libchart-gnuplot-perl xorg-dev xgraph libxt-dev libx11-dev  -y > /dev/null 2>&1
echo "[===============>]"
set -e

echo
echo "Downloading ns-2.35"
wget http://sourceforge.net/projects/nsnam/files/allinone/ns-allinone-2.35/ns-allinone-2.35.tar.gz/download > /dev/null 2>&1

echo "Extracting ns-2.35"
tar -xvf download> /dev/null

echo "Patching ns-2.35"
patch -s -p0 < patch.diff

echo
echo "Compiling and Installing ns-2.35"
if (cd ns-allinone-2.35 && ./install > /home/vagrant/install.log 2>&1) ; then
    echo "Install succeeded"
else
    >&2 echo "Compilation failed. To see compilation log, run 'vagrant ssh' and then view 'install.log'"
    exit 1
fi

export PATH=$PATH:/home/vagrant/ns-allinone-2.35/bin:/home/vagrant/ns-allinone-2.35/tcl8.5.10/unix:/home/vagrant/ns-allinone-2.35/tk8.5.10/unix
export LD_LIBRARY_PATH=/home/vagrant/ns-allinone-2.35/otcl-1.14:/home/vagrant/ns-allinone-2.35/lib
export TCL_LIBRARY_PATH=/home/vagrant/ns-allinone-2.35/tcl8.5.10/library

echo
( cd test-scripts && ./run.sh )
