#!/bin/bash

PROTEX_HOME=/NAS/tools/ip_protex/
PROTEX_ROOT=$PROTEX_HOME/ip_protex_7.1.3/protexIP
BDSTOOL=$PROTEX_ROOT/bin/bdstool
PROTEX_PROJECT_NAME=c_openvkl_20491
SRC_PATH=$CI_PROJECT_DIR/

export _JAVA_OPTIONS=-Duser.home=$PROTEX_HOME/protex_home

# enter source code directory before scanning
cd $SRC_PATH

$BDSTOOL new-project $PROTEX_PROJECT_NAME | tee ip_protex.log
$BDSTOOL analyze | tee -a ip_protex.log

if grep -E "^Files pending identification: [0-9]+$" ip_protex.log
then
    echo "Protex scan FAILED!"
    cat ip_protex.log
    exit 1
fi

echo "Protex scan PASSED!"
exit 0
