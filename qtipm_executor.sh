#!/bin/bash

adb shell "./data/local/tmp/getmonotonic > /tmp/monotonic.txt" &
adb exec-out 'su 0  echo 1 > /dev/qtipm' &
cat ../profiling/power.pbtx | adb -s 9e09341 shell /data/local/tmp/out/android/perfetto -o tmp/new -c - --txt
adb exec-out 'su 0  echo 0 > /dev/qtipm'
adb exec-out 'su 0  cat /dev/qtipm > /tmp/qtipm.txt'

echo "Done"

adb pull /tmp/monotonic.txt ../profiling/qtipm_traces/.
adb pull /tmp/qtipm.txt ../profiling/qtipm_traces/.
adb pull /tmp/new ../profiling/qtipm_traces/.
