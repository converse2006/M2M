rm facedetect
arm-none-linux-gnueabi-g++ opencv-facedetect.cpp -o facedetect `pkg-config --cflags --libs opencv` `pkg-config --cflags --libs zlib` -ljpeg -march=armv4t -static
