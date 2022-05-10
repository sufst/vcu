#!/bin/zsh
###############################################################################
#               :
#   File        :   toolchain-installer-macos.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Toolchain installer for macOS
#               :
###############################################################################

source utility.sh 

ARM_NONE_EABI_LINK="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.pkg"
ARM_NONE_EABI_PATH="/Applications/ARM/bin"

# make sure this is macOS
if [[ $OSTYPE != "darwin"* ]]; then 
    error "Not macOS"
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
        arm-none-eabi-gdb
    )

    TO_INSTALL=""

    for TOOL in $TOOLCHAIN;
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
    DOWNLOAD_LOCATION="$HOME/Downloads"

    # download latest (if the file isn't already there)
    FILE=$(find "$DOWNLOAD_LOCATION" -type f -name "gcc-arm-none-eabi-*" | head -n 1 | grep .)

    if [[ $? != 0 ]]; then 
        info "Downloading: $ARM_NONE_EABI_LINK"
        (cd "$DOWNLOAD_LOCATION"; curl -OsSL "$ARM_NONE_EABI_LINK" > /dev/null) 
    else 
        info "Found file: $FILE"
    fi

    # install
    FILE=$(find "$DOWNLOAD_LOCATION" -type f -name "gcc-arm-none-eabi-*" | head -n 1)
    info "Installing:" "$FILE"
    sudo installer -pkg $FILE -target / > /dev/null
    
    # check for error
    if [[ $? != 0 ]]; then 
        error "Failed to install arm-none-eabi"
        exit 1
    else 
        success "Installed arm-none-eabi"
    fi

    # update path if required
    step "\nChecking arm-none-eabi path..."
    PATHS_TO_ADD=""

    echo "$PATH" | grep "$ARM_NONE_EABI_PATH" > /dev/null

    if [[ $? != 0 ]]; then 
        warning "Did not find $ARM_NONE_EABI_PATH on \$PATH" 
        PATHS_TO_ADD="$PATHS_TO_ADD\nexport PATH=\"$ARM_NONE_EABI_PATH:\$PATH\""
    fi

    if [[ ! -z "$PATHS_TO_ADD" ]]; then 
        
        info "Copying missing paths to clipboard..."
        echo "$PATHS_TO_ADD" | pbcopy  
        
        info "Please paste the copied paths into .zshrc"
        osascript -e 'display alert "Please paste the copied paths into .zshrc"' > /dev/null
        $EDITOR "~/.zshrc"

        source ~/.zshrc 
    fi

    cat ~/.zshrc | grep "$PATHS_TO_ADD" > /dev/null 

    if [[ $? != 0 ]]; then
        error "Updated paths not found in .zshrc"
        exit 1
    else 
        success "Paths up to date"
    fi

fi

# install stlink if required
echo "$TO_INSTALL" | grep "st-" > /dev/null 

if [[ $? == 0 ]]; then 

    step "\nInstalling stlink..."

    # check for homebrew
    which brew > /dev/null 

    if [[ $? != 0 ]]; then 
        error "Homebrew not installed (https://brew.sh/)"
        exit 1
    fi

    # install with homebrew
    brew update 
    brew install stlink

    if [[ $? == 0 ]]; then 
        success "Installed stlink"
    else 
        error "Failed to install stlink"
    fi
    
fi

# check toolchain
step "\nChecking toolchain post installation..."
check_toolchain