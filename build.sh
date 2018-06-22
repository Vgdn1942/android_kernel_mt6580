#!/bin/bash
#
# Copyright (C) 2018 Yuvraj Saxena (abyss10)
# Copyright (C) 2017 Pablo Fraile Alonso (Github aka: Pablito2020)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

# Set Colors! (some of there aren't used, but if you like, you can add it in the echo lines.)
blue='\033[0;34m'
cyan='\033[0;36m'
yellow='\033[0;33m'
green='\033[0;32m'
red='\033[0;31m'
nocol='\033[0m'
orange='\033[0;33m'
light_red='\033[1;31m'
purple='\033[0;35m'
m=make
e=echo

# If the Google toolchain doesn't exist, clone it. 
if [ ! -d gtc ]
then
    $e -e "####################################"
    $e -e "#       TOOLCHAIN NOT FOUND!       #"
    $e -e "####################################"
    $e -e "#${green} Cloning To Use           #"
git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8 gtc
export ARCH=arm CROSS_COMPILE=$PWD/gtc/bin/arm-eabi-
else
export ARCH=arm CROSS_COMPILE=$PWD/gtc/bin/arm-eabi-
fi

# User and Build Host
export KBUILD_BUILD_USER=Yuvraj
export KBUILD_BUILD_HOST=âˆ†Thestral

# Read the lineage/AOSP DEFCONFIG
$e -e "${orange} CONFIGURING Defconfig..."
mkdir -p out
$m p350_defconfig O=out/ >/dev/null

# Build zImage
$e -e "${orange} BUILDING KERNEL FOR P350..."
$m -j4 O=out/ &>> Kernel.log

# Check if there are errors in the kernel
if [ ! -f out/arch/arm/boot/zImage ]
then
    $e -e "${red}############################"
    $e -e "${red}#        BUILD ERROR!      #"
    $e -e "${red}############################"
else

# If the kernel compiles succesfully
$e -e "${green} #########################################"
$e -e "${green} #                                       #"
$e -e "${green} # SUCCESSFULLY BUILDED KERNEL #"
$e -e "${green} #        $[$SECONDS / 60]' minutes '$[$SECONDS % 60]' seconds'       #" 
$e -e "${green} #                                       #"
$e -e "${green} #########################################"
fi
