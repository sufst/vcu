#!/bin/bash
###############################################################################
#               :
#   File        :   makefile.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Makefile code generation
#               :   -> fixes errors in STM32CubeMX generated Makefile
#               :   -> adds SUFST code to includes / sources
#               :
###############################################################################

DEBUG=0
SOURCE_FOLDERS=(SUFST/Src)
INCLUDE_FOLDERS=(SUFST/Inc)
MAKEFILE=Makefile

###############################################################################
# script defines
###############################################################################

TPUT_BLACK=0
TPUT_RED=1
TPUT_GREEN=2
TPUT_YELLOW=3
TPUT_BLUE=4
TPUT_MAGENTA=5
TPUT_CYAN=6
TPUT_WHITE=7
TPUT_RESET=sgr0

###############################################################################
# script task
###############################################################################

function main()
{
    echo
    check_toolchain
    update_sufst_files
    ensure_asm_lowercase
    fix_rtos_port
}

###############################################################################
# checks host system capabilities
###############################################################################

function check_toolchain()
{
    step "Checking toolchain installation"

    # check toolchain
    local TOOLCHAIN=(
        make
        st-flash 
        st-util
        arm-none-eabi-gcc
        arm-none-eabi-ld
        arm-none-eabi-objdump
        arm-none-eabi-objcopy
        arm-none-eabi-size
        arm-none-eabi-gdb
    )

    local WARNINGS=0

    for TOOL in "${TOOLCHAIN[@]}";
    do 
        which $TOOL > /dev/null
        
        if [[ $? != 0 ]]; then 
            info_cross $TOOL
            WARNINGS=$((WARNINGS+1))
        else 
            info_tick $TOOL
        fi
    done

    if [[ $WARNINGS != 0 ]]; then 
        warning $WARNINGS" tool(s) not found"
    fi

    echo 
}

###############################################################################
# update SUFST sources and headers in makefile
###############################################################################

function update_sufst_files()
{
    step "Updating SUFST files"
    local TMP_FILE="Makefile.tmp"
    
    # source files
    for DIR in "${SOURCE_FOLDERS[@]}"
    do 
        # find all sources (recursive)
        local SOURCES=$(find $DIR -type f -name "*.c")
        local INSERT_AT=$(cat "$MAKEFILE" | grep "C_SOURCES =" | sed 's/\\/\\\\/g')

        # add sources to makefile if not already there
        echo "$SOURCES" | while IFS= read -r LINE;  
        do 
            # check if already there
            cat $MAKEFILE | grep "$LINE" > /dev/null
            
            # if not then add the line
            # -> have to escape every / and \ for sed with a \
            if [[ $? != 0 ]]; then 
                local ESCAPED_LINE=$(echo "$LINE \\\\" | sed 's/\//\\\//g')
                cat "$MAKEFILE" | sed "s/$INSERT_AT/$INSERT_AT\n$ESCAPED_LINE/g" > $TMP_FILE
                cp "$TMP_FILE" "$MAKEFILE"
                added_file $LINE
            fi 
        done 

        # remove source files corresponding to this directory that have been deleted
        local MAKEFILE_SOURCES=$(cat $MAKEFILE | grep $DIR | sed 's/ \\//g')

        echo "$MAKEFILE_SOURCES" | while IFS= read -r SOURCE;  
        do 
            # check if the source file is in the makefile but not in this directory
            # remove from the makefile if so
            if [[ ! -f "$SOURCE" ]]; then
                local LINE_TO_REMOVE=$(cat "$MAKEFILE" | grep "$SOURCE" | sed 's/\\/\\\\/g'  | sed 's/\//\\\//g')
                cat "$MAKEFILE" | sed "/$LINE_TO_REMOVE/d" > $TMP_FILE
                cp "$TMP_FILE" "$MAKEFILE"
                removed_file $SOURCE
            fi
        done 
    done

    # include directories
    for DIR in "${INCLUDE_FOLDERS[@]}"
    do 
        # find all subdirectories + parent directory
        local INCLUDES=$(find $DIR -type d)
        local INSERT_AT=$(cat "$MAKEFILE" | grep "C_INCLUDES =" | sed 's/\\/\\\\/g')

        # add include directories to makefile if not already there
        echo "$INCLUDES" | while IFS= read -r INCLUDE;  
        do 
            # check if already there
            cat $MAKEFILE | grep "$INCLUDE" > /dev/null
            
            # if not then add the line
            # -> have to escape every / and \ for sed with a \
            if [[ $? != 0 ]]; then 
                local ESCAPED_LINE=$(echo "-I$INCLUDE \\\\" | sed 's/\//\\\//g')
                cat "$MAKEFILE" | sed "s/$INSERT_AT/$INSERT_AT\n$ESCAPED_LINE/g" > $TMP_FILE
                cp "$TMP_FILE" "$MAKEFILE"
                added_file $INCLUDE
            fi 
        done 

        # remove include directories corresponding to this directory that have been deleted
        local MAKEFILE_INCLUDES=$(cat $MAKEFILE | grep "$DIR" | sed 's/ \\//g' | sed 's/-I//g')

        echo "$MAKEFILE_INCLUDES" | while IFS= read -r INCLUDE;  
        do 
            # check if the include directory is in the makefile but not in this directory
            # remove from the makefile if so
            if [[ ! -d "$INCLUDE" ]]; then
                local LINE_TO_REMOVE=$(cat "$MAKEFILE" | grep "$INCLUDE" | sed 's/\\/\\\\/g'  | sed 's/\//\\\//g')
                cat "$MAKEFILE" | sed "/$LINE_TO_REMOVE/d" > $TMP_FILE
                cp "$TMP_FILE" "$MAKEFILE"
                removed_file $INCLUDE
            fi
        done 
    done

    # check if any changes were actually made
    if [[ ! -f "$TMP_FILE" ]]; then 
        info "(no changes)"
    else 
        rm $TMP_FILE
    fi
    echo
}

###############################################################################
# fixes bug in STM32CubeMX that causes incorrect RTOS port to be used
###############################################################################
function fix_rtos_port()
{
    # first check if anything needs to be done 
    cat "$MAKEFILE" | grep "/iar/" > /dev/null 

    if [[ $? != 0 ]]; then 
        return
    fi

    step "Fixing RTOS port"
    local RTOS_PATH="Middlewares/ST/threadx/"
    local TMP_FILE=Makefile.tmp
    
    # remove IAR sources which are not needed
    local NON_GNU_SOURCES=(
        tx_thread_interrupt_disable
        tx_thread_interrupt_restore
        tx_iar
    )

    for SOURCE in $"${NON_GNU_SOURCES[@]}" 
    do 
        cat "$MAKEFILE" | grep $SOURCE > /dev/null 
        if [[ $? == 0 ]]; then 
            local LINE_TO_REMOVE=$(cat "$MAKEFILE" | grep "$SOURCE" | sed 's/\\/\\\\/g'  | sed 's/\//\\\//g')
            cat "$MAKEFILE" | sed "/$LINE_TO_REMOVE/d" > $TMP_FILE
            mv "$TMP_FILE" "$MAKEFILE"
            removed_file $SOURCE
        fi
    done

    # replace IAR port paths with GNU port paths
    cat "$MAKEFILE" | sed "s/\/iar\//\/gnu\//g" > $TMP_FILE
    mv "$TMP_FILE" "$MAKEFILE"
    modified_file "/iar/ -> /gnu/"

    echo 
}

###############################################################################
# ensures that assembly files have lowercase .s extension
###############################################################################

function ensure_asm_lowercase()
{
    # first check if anything needs to be done
    local RUN_MAKEFILE=0
    local RUN_ASM_FILES=0

    cat "$MAKEFILE" | grep "\.S" > /dev/null
    if [[ $? == 0 ]]; then 
        RUN_MAKEFILE=1
    fi

    find . -type f -name "*.S" | grep . > /dev/null
    if [[ $? == 0 ]]; then 
        RUN_ASM_FILES=1
    fi

    if [[ $RUN_MAKEFILE == 0 && $RUN_ASM_FILES == 0 ]]; then 
        return
    fi

    step "Renaming .S -> .s:"
    local TMP_FILE="Makefile.tmp"

    # .S -> .s in makefile
    if [[ $RUN_MAKEFILE != 0 ]]; then
        cat "$MAKEFILE" | sed "s/\.S/\.s/g" > $TMP_FILE
        mv "$TMP_FILE" "$MAKEFILE"
        modified_file "$MAKEFILE .S -> .s"
    fi

    # .S -> .s for actual file extensions recursively
    if [[ $RUN_ASM_FILES != 0 ]]; then 
        find ./ -depth -name "*.S" -exec sh -c 'mv "$1" "${1%.S}.s"; echo "$1" | cut -c 4-' _ {} \;
    fi
    
    echo
}

###############################################################################
# printing
###############################################################################

function step() 
{
    tput setaf $TPUT_MAGENTA
    tput bold
    echo -e $*
    tput $TPUT_RESET
}

function info()
{
    echo -e $*
}

function info_tick()
{
    tput setaf $TPUT_GREEN 
    printf "\342\234\224 "
    tput $TPUT_RESET
    info $*
}

function info_cross()
{
    tput setaf $TPUT_RED
    printf "\342\234\227 "
    tput $TPUT_RESET
    info $*
}

function error()
{
    tput setaf $TPUT_RED
    echo "Error: "$*
    tput $TPUT_RESET
    exit 1
}

function warning()
{
    tput setaf $TPUT_YELLOW 
    echo "Warning: "$* 
    tput $TPUT_RESET
}

function success()
{
    tput setaf $TPUT_GREEN 
    echo $*
    tput $TPUT_RESET
}

function added_file()
{
    tput setaf $TPUT_GREEN 
    echo -n "A: "
    tput $TPUT_RESET 
    echo $*
}

function removed_file()
{
    tput setaf $TPUT_RED 
    echo -n "D: "
    tput $TPUT_RESET 
    echo $*
}

function modified_file()
{
    tput setaf $TPUT_CYAN 
    echo -n "M: "
    tput $TPUT_RESET 
    echo $*
}

###############################################################################
# driver code
###############################################################################

main "$@"; exit