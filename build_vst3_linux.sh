#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/code/vst/install_dependencies.sh
$SCRIPT_DIR/code/vst/make_vst3.sh CONFIG=Release
