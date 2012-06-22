make clean
make
adb -s emulator-5554 push surveillance /data
adb -s emulator-5554 push haarcascade_frontalface_alt.xml /data
adb -s emulator-5556 push surveillance /data
adb -s emulator-5556 push jay.jpeg /data

