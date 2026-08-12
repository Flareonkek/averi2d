A 2d platforming game prototype, I'm still just getting started making it.
See https://averi.foo/anthro/thread/135401

Files inside folder "raylib" are not mine, but someone else's code I'm using for windows and grapics.
See https://www.raylib.com/ for details.

TO INSTALL

After putting all these files on your computer
(In the terminal enter: git clone [The URL from the green "Code" button in the top right of the github page] ),
there are 2 extra little steps you have to do to get it to work.

Give compile.sh permission to execute, right-click it, hit "Properties" and check a box for that.
(Or you can enter chmod +x compile.sh in the terminal, that does the same thing)

Install all these prerequisites for raylib to work (You only gotta do this once if you haven't already)
By entering this in the terminal (On other rare/weird linux versions it may be some different command, but it'll be the same stuff you're ultimately trying to install.)
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev

Now just run
./compile.sh
which will create an executable file "main", which you can run by entering
./main
And the game should start up in a new window.
