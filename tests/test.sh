#!/bin/sh
set -u

QREAN=${QREAN:-../build/system/qrean}
QREAN_DETECT=${QREAN_DETECT:-../build/system/qrean-detect}
error=0

check() {
	echo -n "  $2 ... "
	if diff -q "$1" "$2" > /dev/null; then
		printf "\033[32mPASS\033[m\n"
	else
		printf "\033[31mFAILED\033[m\n"
		error=1
	fi
}

echo "Generation:"
${QREAN} -t qr   -f txt -l L -8 -p 4 -p 4 "Hello" | check - qr-1.txt
${QREAN} -t mqr  -f txt -l L -8 -m 1 -p 2 "test"  | check - mqr-1.txt
${QREAN} -t rmqr -f txt "Hello" | check - rmqr-1.txt
${QREAN} -t tqr  -f txt "000000000000" | check - tqr-000000000000.txt

echo "Detection:"
${QREAN_DETECT} github-libqrean-qr.png | check - github-libqrean-qr.txt

exit ${error}
