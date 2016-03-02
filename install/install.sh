#!/bin/bash

f_with_deps= #install dependancies

usage="\
Usage: $0 [OPTION]

Options:
    --with_deps     Try to install all needed dependancies
"

while test $# -ne 0; do
    case $1 in    
    --with_deps)
        f_with_deps=true
    ;;
    
    --help) echo "$usage"; exit $?;;

    --version) echo "$0 $scriptversion"; exit $?;;

    --) shift
    break;;

    -*) echo "$0: invalid option: $1" >&2
    exit 1;;

    *)  break;;
    esac  
    
    shift
done

if test -n "$f_with_deps"; then
    apt-get install -y openssl libssl-dev
    apt-get install -y libboost-all-dev
fi

