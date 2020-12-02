#!/bin/bash
set -e
export KW_ISSUES_OUTPUT_PATH=$CI_PROJECT_DIR/issues.out
export KW_BUILD_NUMBER=$(cat $CI_PROJECT_DIR/kw_build_number)
echo "Checking for issues in $KW_BUILD_NUMBER ..."
no_proxy=$KW_SERVER_IP curl -f --data "action=search&project=$KW_PROJECT_NAME&query=build:'$KW_BUILD_NUMBER'%20severity:Error,Critical%20status:Analyze,Fix&user=$KW_USER&ltoken=$KW_LTOKEN" http://$KW_SERVER_IP:$KW_SERVER_PORT/review/api -o $KW_ISSUES_OUTPUT_PATH
getCriticalCount() {
        cat $KW_ISSUES_OUTPUT_PATH | wc -l
}
if [ -f $KW_ISSUES_OUTPUT_PATH ]; then
        echo "Issues found - $(getCriticalCount) in $KW_BUILD_NUMBER";
        cat $KW_ISSUES_OUTPUT_PATH
        exit 1;
else
        echo "No issues were found in $KW_BUILD_NUMBER"
fi
