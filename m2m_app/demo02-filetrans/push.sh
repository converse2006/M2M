make clean
make
adb -s emulator-5554 push vnd_api /data
adb -s emulator-5556 push vnd_api /data
adb -s emulator-5556 push lin.jpeg /data
