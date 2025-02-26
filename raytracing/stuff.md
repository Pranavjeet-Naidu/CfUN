install : SDL2 : 
sudo apt-get install libsdl2-dev

for fedora systems : sudo dnf install SDL2-devel

also , command to compile : gcc main.c -o main $(sdl2-config --cflags --libs) -lm