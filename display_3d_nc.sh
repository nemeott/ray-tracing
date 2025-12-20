g++ -std=c++17 display_3d_nc.cpp -o display_3d_nc -O3 \
    $(pkg-config --cflags --libs notcurses++)
./display_3d_nc