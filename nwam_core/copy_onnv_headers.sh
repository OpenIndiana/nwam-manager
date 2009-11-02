#!/bin/bash 

echo_cp() {
    echo "cp '${1}' \\
    '${2}'"
    cp ${1} "${2}"
}

echo "Please input nwam gate (default is $HOME/nwam-gate):"
read NWAM_PHASE1_REPO

[ -z "$NWAM_PHASE1_REPO" ] && NWAM_PHASE1_REPO=$HOME/nwam-gate

CLONE_HEADERS="usr/src/lib/libnwam/common/libnwam.h
usr/src/lib/libinetcfg/common/inetcfg.h
usr/src/lib/libdladm/common/*.h"

CLONE_SYS_HEADERS="usr/src/uts/common/sys/mac.h
usr/src/uts/common/sys/mac_flow.h
usr/src/uts/common/sys/dld.h
usr/src/uts/common/sys/dls_mgmt.h"

if [ -n "$1" -a -d "${1}/usr/src/lib/libnwam" ]; then
    REPO="$1"
elif [ -n "$1" ]; then
    echo "$1 doesn't appear to be a valid ONNV clone"
    exit 1
else
    echo "Using internally configured repo, to override specify on command-line"
    REPO=${NWAM_PHASE1_REPO}
fi

echo "**************************************************************************"
echo "Using ONNV source repo of ${REPO}"
echo "**************************************************************************"

if [ ! -d "${REPO}/usr/src/lib/libnwam" ]; then
    echo "Repository doesn't appear to exist, exiting...."
    exit 1
fi

if [ ! -d usr/include/sys ]; then
    mkdir -p usr/include/sys
fi

for f in ${CLONE_HEADERS}
do
    echo_cp "${REPO}/${f}" usr/include/
done

for f in ${CLONE_SYS_HEADERS}
do
    echo_cp "${REPO}/${f}" usr/include/sys/
done

