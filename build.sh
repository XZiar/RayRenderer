#!/bin/bash
set -e
Action=$1
Project=$2
Target=${3:-"Debug"}
Paltform=${4:-"x64"}
ProjDir=$(pwd)/

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
    rebuild)
        echo "rebuild [$Project]"
        clean;
        build;
        ;;
    help)
        echo "build.sh [<build|clean|rebuild>] <project> [<Debug|Release>] [<x64|x86>]"
        ;;
esac
exit 0