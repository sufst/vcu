#!/bin/bash
###############################################################################
#               :
#   File        :   compile-commands.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Generates compile commands for VS Code
#               :
###############################################################################

SCRIPTPATH="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)"
source "$SCRIPTPATH/utility.sh"

TMP_FILE=compile_commands.tmp
JSON_FILE=compile_commands.json

step "Generating compile_commands.json"

# makefile compiler calls
# -> .c and .s
# -> excluding final linking step
info "Cleaning build directory..."
make clean -s > /dev/null

info "Running make..."
make -j$(nproc) | grep "arm-none-eabi-gcc" | sed '$d' > $TMP_FILE

# remove existing file
if [[ -f "$JSON_FILE" ]]; then 
    rm "$JSON_FILE"
fi

# start of JSON file
echo -e "[" >> "$JSON_FILE"

# go through each command
info "Parsing command information..."

IFS=$'\n'; set -f
for COMMAND in $(cat < "$TMP_FILE"); do

    # source file being compiled (.c or .s)
    SOURCE_FILE=$(echo "$COMMAND" | tr " " "\n" | grep -e ".*\.c$" -e ".*\.s$")
    
    # directory of build folder
    REPO_ROOT_DIR="$(git rev-parse --show-toplevel)"
    EMBEDDED_DIR="$REPO_ROOT_DIR/embedded"
    BUILD_DIR="$EMBEDDED_DIR/build"

    # escape quotes in the command 
    ESCAPED_COMMAND=$(echo "$COMMAND" | sed 's/"/\\"/g')

    # append JSON object describing command to file
    JSON="\t{\n\t\t\"directory\": \"$BUILD_DIR\",\n\t\t\"command\": \"$ESCAPED_COMMAND\",\n\t\t\"file\": \"$EMBEDDED_DIR/$SOURCE_FILE\"\n\t},"
    echo -e "$JSON" >> "$JSON_FILE"

done

# end of JSON file
echo -e "]" >> "$JSON_FILE"

# clean up
rm "$TMP_FILE"