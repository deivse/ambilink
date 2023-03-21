#!/bin/bash
INIT_DIR=$( pwd )
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

set -e
# keep track of the last executed command
trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
# echo an error message before exiting
trap 'echo "\"${last_command}\" command failed with exit code $?."' EXIT

cd projucer_project/Builds/LinuxMakefile/

# generate compile_commands.json from Makefile build system (for clangd support)
bear --output ../../compile_commands.json -- make -j8 CONFIG=Debug $@
cd $INIT_DIR

trap EXIT


