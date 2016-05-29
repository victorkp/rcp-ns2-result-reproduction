#!/bin/bash

set -e

# Install virtualbox and vagrant if needed
if [ ! command -v vagrant > /dev/null 2>&1 ] || [ ! command -v virtualbox > /dev/null 2>&1 ]; then
    echo "Vagrant or Vagrant is not installed"
    if [ -f /etc/redhat-release ]; then
        echo "Installing VirtualBox and Vagrant"
        sudo dnf install vagrant VirtualBox -y
    elif [ -f /etc/lsb-release ]; then
        sudo apt-get update > /dev/null 2>&1
        sudo apt-get install vagrant virtualbox -y
    else
        echo "Cannot determine Linux distribution, please install Vagrant and Virtualbox, then re-run this script"
        echo "If you are using Mac, please install VirtualBox if it is not already installed. Mac users may download Vagrant here: https://releases.hashicorp.com/vagrant/1.8.1/vagrant_1.8.1.dmg or use Homebrew to install (homebrew cask install vagrant)"
    fi

fi

mkdir -p test-output

# Force stop vagrant VM, if any exists
# vagrant destroy may error if no vagrant VM is 
# currently running or suspended, so temporarily disable
# set -e for early exit on error
set +e
vagrant destroy -f
set -e

# Download (if needed), provision, and run test script
vagrant up

# Clean up vagrant VM
vagrant destroy -f
