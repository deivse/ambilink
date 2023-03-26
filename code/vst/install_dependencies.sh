#!/bin/bash
INIT_DIR=$( pwd )
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

ensure_installed() {
    dpkg -l | grep -q "[[:space:]]$1\(:[^[:space:]]\+\)\?[[:space:]]";
    if [ $? -ne 0 ]; then
        echo "The package $1 will now be installed. You may be prompted for your password."
        sudo apt-get -y install $1
    fi;
}

echo; echo "================================================="
echo "======== Installing package dependencies ========"
echo "================================================="; echo

ensure_installed bear # to generate compile_commands.json from Makefile build
ensure_installed python3
ensure_installed python3-pip
ensure_installed cmake
ensure_installed gcc-11
ensure_installed gfortran # to build openblas

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

if pip3 list 2>/dev/null | grep -q '^conan[[:space:]]*2'; then
    echo "ERROR: conan 2.X installation detected. Only conan 1.X is currently supported.";
    exit 1;
fi;

pip3 install "conan<2.0"
conan=$(python3 -m site --user-base)/bin/conan
for build_type in Release Debug; do
    $conan install . -if build -of build --profile=./conan_profiles/ubuntu20.conanprofile --build=missing -s build_type=$build_type
done

cd $INIT_DIR
########################
