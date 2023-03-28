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
    return 0
}

echo; echo "================================================="
echo "======== Installing package dependencies ========"
echo "================================================="; echo

### The goal is to consume as much dependencies as
### possible via conan, but it's hard...

ensure_installed python3 # to get conan
ensure_installed python3-pip # to get conan
ensure_installed cmake # we use a conan tools_requires for cmake, but some conan packages don't do that..
ensure_installed gcc-11 # the version of juce used doesn't like gcc-12
ensure_installed gfortran-11 # to build openblas - the recipe in conan center doesn't
                             # include a build requirement for gfortran...

### JUCE dependencies: ### (https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md)
juce_dependencies="pkg-config libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype6-dev\
                   libwebkit2gtk-4.0-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev\
                   libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev"
for pkg in $juce_dependencies; do
    ensure_installed "$pkg"
done

set -e

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

$conan install . -if build --profile=./conan_profiles/ubuntu20 --build=missing

cd $INIT_DIR
########################
