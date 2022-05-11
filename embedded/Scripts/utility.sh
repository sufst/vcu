###############################################################################
#               :
#   File        :   utility.sh
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Miscellaneous shell script utility functions
#               :
###############################################################################

# tput terminal colours
TPUT_BLACK=0
TPUT_RED=1
TPUT_GREEN=2
TPUT_YELLOW=3
TPUT_BLUE=4
TPUT_MAGENTA=5
TPUT_CYAN=6
TPUT_WHITE=7
TPUT_RESET=sgr0

# print functions
function step() 
{
    tput setaf $TPUT_MAGENTA
    tput bold
    echo $*
    tput $TPUT_RESET
}

function header()
{
    tput setaf $TPUT_WHITE
    tput bold
    echo $* 
    tput $TPUT_RESET
}

function info()
{
    echo $*
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
    printf "Error: "
    tput $TPUT_RESET
    echo $*
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