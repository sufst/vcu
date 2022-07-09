###############################################################################
#               :
#   File        :   toolchain.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Toolchain checking and installation script
#               :
###############################################################################

source utility.sh

###############################################################################
# function to check toolchain
###############################################################################

function check_toolchain()
{
    step "Checking toolchain installation"

    # list of toolchain requirements
    local TOOLCHAIN=(
        make
        st-flash 
        st-util
        arm-none-eabi-gcc
        arm-none-eabi-ld
        arm-none-eabi-objdump
        arm-none-eabi-objcopy
        arm-none-eabi-size
    )

    # check for each tool
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

    # warn
    if [[ $WARNINGS != 0 ]]; then 
        warning $WARNINGS" tool(s) not found"
        exit 1
    fi
}

###############################################################################
# auto installer wrapper
###############################################################################

function install_toolchain()
{
    step "Attempting to auto-install toolchain..."

    # check for supported OS
    OS=$(uname)
    info "OS type: $OS" 

    case "$OS" in 
        
        Darwin)
            success "Running macOS installer\n"
            /bin/zsh -c ./toolchain-installer-macos.zsh
            ;;

        Linux)
            DISTRO=$(cat /etc/os-release | grep "ID=" | head -n 1 | sed 's/ID=//g')
            info "Distribution: $DISTRO"
            
            if [[ "$DISTRO" == "ubuntu" ]]; then 
                success -e "Running Ubuntu installer\n"
                /bin/bash -c ./toolchain-installer-ubuntu.sh
            else 
                error "No installer for $OS $DISTRO"
                exit 1
            fi
            ;;

        *)
            error "No installer for $OS"
            exit 1
            ;;
    esac
}

###############################################################################
# script driver code
###############################################################################

if [[ $# == 0 ]]; then 
    error "No arguments given"
    exit 1
fi

while [[ $# -gt 0 ]]; do
  case $1 in
    check)
      check_toolchain
      shift
      ;;
    install)
      install_toolchain
      shift
      ;;
    *)
      error "Unknown option $1"
      exit 1
      ;;
  esac
done