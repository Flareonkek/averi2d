# COMPILE THE DEMO PROGRAM
# There are a bunch of flags allowing gcc to use raylib in conjunction with my files

gcc main.c -o main -I./raylib -L./raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# (And like with any program you can then run it with ./main)
