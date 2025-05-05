#!/bin/sh

# Root directory for installation.
# install_root=$HOME/usr

if test -z "$install_root"; then
    echo "Please, set install_root in install.sh before running install.sh."
    exit 1
fi

# Perform installation under installation directory.
cp build/libcomo.so ${install_root}/lib
cp src/como.h ${install_root}/include
cp man/como.3 ${install_root}/man/man3
