#!/usr/bin/env bash

set -eou pipefail

pushd . > /dev/null
cd "$(dirname "$0")"
BASEDIR="$PWD"
popd > /dev/null

export WINEARCH=win32
export WINEPREFIX="$BASEDIR/.wine"
export WINEDEBUG=-all

if ! [ -d "$WINEPREFIX" ]; then
    if ! which winetricks > /dev/null 2>&1; then
        echo "winetricks not installed" >&2
        exit 1
    fi
    wineboot -i || exit 1
    winetricks winxp || exit 1
fi

exec wine "$@"


