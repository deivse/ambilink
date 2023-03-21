#!/bin/bash
########################
CONFIG=Release
########################

INIT_DIR=$( pwd )
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

ensure_installed() {
    dpkg -l | grep -q "[[:space:]]$1"
    if [ $? -ne 0 ]; then
        echo "The package $1 will now be installed. You may be prompted for your password."
        sudo apt-get install $1
    fi;
}

echo; echo "================================================="
echo "======== Installing package dependencies ========"
echo "================================================="; echo

ensure_installed bear # to generate compile_commands.json from Makefile build

### JUCE dependencies: ###
ensure_installed pkg-config 
ensure_installed libfreetype6-dev
ensure_installed libwebkit2gtk-4.0-dev

echo; echo "================================================="
echo "============= Cloning GIT submodules ============"
echo "================================================="; echo

cd ../..
git submodule init
git submodule update
cd code/vst

echo; echo "================================================="
echo "=========== Installing conan packages ==========="
echo "================================================="; echo

conan install . -if build --profile=./conan_profiles/ubuntu20.conanprofile --build=missing

echo; echo "================================================="
echo "============= Building dependencies ============="
echo "================================================="; echo
cmake -S . -B build
cd build; cmake --build . --config $CONFIG

cd $INIT_DIR
########################
