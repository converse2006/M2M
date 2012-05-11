cd node01
make clean
make
adb push vnd_api01 /data
cd ../

cd node02
make clean
make
adb push vnd_api02 /data
cd ../

cd node03
make clean
make
adb push vnd_api03 /data
cd ../

cd node04
make clean
make
adb push vnd_api04 /data
cd ../

cd node05
make clean
make
adb push vnd_api05 /data
cd ../
