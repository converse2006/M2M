make clean
make
#k = $1 * 2
#for(( i=54; i<54 + $k; i=i+2 ))
#do
#    adb -s emulator-55$i push multihops /data
#done

adb -s emulator-5554 push multihops /data
adb -s emulator-5556 push multihops /data
adb -s emulator-5558 push multihops /data
adb -s emulator-5560 push multihops /data
adb -s emulator-5562 push multihops /data
adb -s emulator-5564 push multihops /data
adb -s emulator-5564 push lin.jpeg /data

