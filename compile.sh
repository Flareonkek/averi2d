# COMPILE THE DEMO PROGRAM
# There are a bunch of flags allowing gcc to find raylib's data and shit.
# This is after also building raylib in the folder out in the c folder,
# If you arrange your folders differently you will have to change the paths
# in the two arguments which currently point to ../raylib/src

# (The lack of a leading / means these are relative paths, with respect to
# the folder this command is run in, and the .. gets back out to the c folder.)

gcc main.c -o main -I../raylib/src -L../raylib/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# (And like with any program you can then run it with ./main)
