#!/bin/bash
###############################################################################
#               :
#   File        :   toolchain-installer-macos.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Toolchain installer for Ubuntu Linux
#               :
###############################################################################

source utility.sh

STLINK_URL="https://github.com/stlink-org/stlink/releases/download/v1.7.0/stlink_1.7.0-1_amd64.deb"

# make sure this is Ubuntu Linux
if [[ $OSTYPE != "linux"* ]]; then 
    error "Not Linux"
    exit 1
fi

cat /etc/os-release | grep "Ubuntu" > /dev/null

if [[ $? != 0 ]]; then 
    error "Not Ubuntu"
    exit 1
fi

# function to check toolchain
function check_toolchain()
{
    TOOLCHAIN=(
        make
        st-flash 
        st-util
        arm-none-eabi-gcc
        arm-none-eabi-ld
        arm-none-eabi-objdump
        arm-none-eabi-objcopy
        arm-none-eabi-size
    )

    TO_INSTALL=""

    for TOOL in "${TOOLCHAIN[@]}";
    do 
        which $TOOL > /dev/null
        
        if [[ $? != 0 ]]; then 
            info_cross $TOOL
            TO_INSTALL="$TO_INSTALL ""$TOOL"
        else 
            info_tick $TOOL
        fi
    done

    echo "$TO_INSTALL" | grep . > /dev/null 

    if [[ $? != 0 ]]; then 
        success "All tools installed."
        exit 0
    fi
}

# check what is already installed
# TO_INSTALL will then contain list of missing tools
step "Checking for existing tools..."
check_toolchain

# install ARM toolchain if required
echo "$TO_INSTALL" | grep "arm-none-eabi" > /dev/null

if [[ $? == 0 ]]; then 
    
    step "\nInstalling arm-none-eabi..."
    sudo apt-get update 
    sudo apt-get install gcc-arm-none-eabi

    if [[ $? == 0 ]]; then 
        success "Installed arm-none-eabi"
    else 
        error "Failed to install arm-none-eabi"
    fi  
fi

# install stlink if required
echo "$TO_INSTALL" | grep "st-" > /dev/null

if [[ $? == 0 ]]; then 

    step "\nInstalling stlink..."
    DOWNLOAD_LOCATION="$HOME/Downloads"

    # download (if the file isn't already there)
    FILE=$(find "$DOWNLOAD_LOCATION" -type f -name "stlink*" | head -n 1 | grep .)

    if [[ $? != 0 ]]; then 
        info "Downloading: $STLINK_URL"
        (cd "$DOWNLOAD_LOCATION"; wget -q "$STLINK_URL" > /dev/null) 
    else 
        info "Found file: $FILE"
    fi

    # install
    FILE=$(find "$DOWNLOAD_LOCATION" -name "stlink*" | head -n 1)
    info "Installing:" "$FILE"
    sudo dpkg -i "$FILE"

    # TODO(?): do paths need to be updated here?

fi

# check toolchain
step "\nChecking toolchain post installation..."
check_toolchain