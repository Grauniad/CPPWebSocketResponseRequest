#!/bin/bash

DEPS_BUILD=$PWD/deps/build
if [[ "$1" == "" ]]; then
    echo "No install directory provided, falling back to: $DEPS_BUILD"
else
    DEPS_BUILD=$1
fi

DEPS_CMAKE_DEPO=$PWD/deps/build/lib/cmake
mkdir -p $DEPS_BUILD

if [[ -e CPPWebSocketResponseRequestConfig.cmake ]]; then
    echo "Building dependencies..."
else
    echo "This script should be run in the home directory of the project"
    exit 1
fi

if [[ -e deps/FixedJSONParserCPP ]]; then
    echo "Existing FixedJSONParserCPP directory, no need to clone"
else
    git clone https://github.com/Grauniad/FixedJSONParserCPP.git deps/FixedJSONParserCPP || exit 1
fi

if [[ -e deps/websocketpp ]]; then
    echo "Existing websocketpp directory, no need to clone"
else
    git clone https://github.com/zaphoyd/websocketpp.git deps/websocketpp || exit 1
fi



deps=(FixedJSONParserCPP);
for dep in ${deps[@]} ; do
    mkdir -p deps/$dep/build

    pushd deps/$dep || exit 1

    if [[ -e buildDeps.sh ]]; then
        ./buildDeps.sh $DEPS_BUILD || exit 1
    fi
    git pull
    pushd build || exit 1

    cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH:PATH=$DEPS_CMAKE_DEPO" "-DCMAKE_INSTALL_PREFIX:PATH=$DEPS_BUILD" .. || exit 1
    make -j 3 || exit 1
    make install || exit 1

    popd || exit 1
    popd || exit 1
    done
exit 1
