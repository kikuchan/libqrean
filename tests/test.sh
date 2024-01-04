#!/bin/sh
set -u

QREAN=${QREAN:-../cli/qrean/qrean}
QREAN_DETECT=${QREAN_DETECT:-../cli/qrean/qrean-detect}
error=0

function check() {
	echo -n "$2 ... "
	if diff -q "$1" "$2" > /dev/null; then
		printf "\033[32mPASS\033[m\n"
	else
		printf "\033[31mFAILED\033[m\n"
		error=1
	fi
}

${QREAN} -t rmqr -f TXT Hello | check - rmqr-1.txt

exit ${error}
