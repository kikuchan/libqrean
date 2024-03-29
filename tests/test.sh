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

check_contains() {
	echo -n "  $2 ... "
	if grep -q $(cat "$2") "$1" > /dev/null; then
		printf "\033[32mPASS\033[m\n"
	else
		printf "\033[31mFAILED\033[m\n"
		error=1
	fi
}

echo "Generation:"
${QREAN} -t qr   -f txt -l L -8 -p 4 -p 4 "Hello" | check - qr-1.txt
${QREAN} -t mqr  -f txt -l L -8 -m 1 -p 2 "test"  | check - mqr-1.txt
${QREAN} -t rmqr -f txt -v AUTO-W -p 4 "Hello" | check - rmqr-1.txt
${QREAN} -t tqr  -f txt -p 4 "000000000000" | check - tqr-000000000000.txt
${QREAN} -t qr   -f txt -l L -8 `cat qr-8bit-max.src` | check - qr-8bit-max.txt

echo "Detection:"
${QREAN_DETECT} github-libqrean-qr.png | check_contains - github-libqrean-qr.txt
${QREAN_DETECT} PXL_20240109_050512422.png | check_contains - PXL_20240109_050512422.txt
${QREAN_DETECT} PXL_20240109_050512422_lr.png | check_contains - PXL_20240109_050512422_lr.txt
${QREAN_DETECT} PXL_20240109_050512422_r90.png | check_contains - PXL_20240109_050512422_r90.txt

echo "Kanji:"
${QREAN} -UK $(cat kanji.txt) | ${QREAN_DETECT} | check - kanji.txt

exit ${error}
