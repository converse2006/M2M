make clean
make
cp surveillance surveillance_1
cp surveillance surveillance_2 
adb -s emulator-5554 push surveillance_1 /data
adb -s emulator-5554 push haarcascade_frontalface_alt.xml /data
adb -s emulator-5554 push /home/converse/ANDROID/m2m_android/android.paslab/set_cmd/set /data

adb -s emulator-5556 push /home/converse/ANDROID/m2m_android/android.paslab/set_cmd/set /data
adb -s emulator-5556 push surveillance_2 /data
adb -s emulator-5556 push $1 /data
cp surveillance_1 /home/converse/ANDROID/m2m_android/android.paslab/set_out
cp surveillance_2 /home/converse/ANDROID/m2m_android/android.paslab/set_out

cp surveillance_1 /home/converse/ANDROID/m2m_android/android.paslab/out/target/product/generic/data/app
cp surveillance_2 /home/converse/ANDROID/m2m_android/android.paslab/out/target/product/generic/data/app
