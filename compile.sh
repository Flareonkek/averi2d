# COMPILE THE DEMO PROGRAM

# Firstly, installing all this stuff only needs to be done once if you haven't already on this computer, as a prerequisite, but if you compile it again itll just do nothing I think.

#(COMMENTED OUT SO IT STOPS ASKING ME FOR MY PASSWORD WHILE I REBUILD IT OVER AND OVER WHILE WORKING ON THE GAME)
#sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev

#(SAME HERE)
# You also need this in the extremely unlikely case you don't already have it, this makes sure you have gcc
#sudo apt install build-essential

# Here is the part that actually compiles the program using gcc. There are a bunch of flags linking raylib and its requirements

gcc src/main.c -o main -I./src/raylib -L./src/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# (And like with any program you can then run it with ./main)
