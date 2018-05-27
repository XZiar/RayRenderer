#!/bin/bash
set -e
Project=$1
Action=${2:-"build"}
Target=${3:-"Debug"}
Paltform=${4:-"x64"}
ProjDir=$(pwd)/

if [ "$Project" = "help" ]
then
    echo "build.sh <project> [<build|clean>] [<Debug|Release>] [<x64|x86>]"
    exit 0
fi

function build()
{
    make BOOST_PATH="$CPP_DEPENDENCY_PATH/include" PLATFORM=$Paltform TARGET=$Target PROJPATH=$ProjDir -j4
}

function clean()
{
    make clean
}

echo "Project Directory:$ProjDir"
echo "Build:$Project"
echo "Target:$Target"
echo "Platform:$Paltform"
cd $Project

case $Action in
    build)
        echo "build [$Project]"
        build;
        ;;
    clean)
        echo "clean [$Project]"
        clean;
        ;;

esac
exit 0