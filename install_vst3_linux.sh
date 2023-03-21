#!/bin/bash
INIT_DIR=$( pwd )
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

set -e

echo "Removing old VST3 instance..."
rm -r ~/.vst3/Ambilink.vst3/
echo "Copying new VST3 instance..."
cp -r ./code/vst/build/Ambilink.vst3 ~/.vst3/Ambilink.vst3
echo ">> Success!"

cd $INIT_DIR

trap EXIT



