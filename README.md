# protoengine

Proto Engine aims to be a simple self contained 2d custom game engine. 
The main goal is to provide a simple no fuzz prototyping tool in c.

Written in c and opengl using the awesome tcc compiler (Tiny C compiler).
For beginners or someone that wants a simple/fast way to start coding a prototype.

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
Installation:

Simply download the repository and place in c:\proto. You're good to go.

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

To create new Prototype:

- Copy paste the template folder and rename to anything you want
- The entire source is in source/external and the actual game code should in the source
- Start messing with source\main.c
- Place any image files (png 32bit) in build/res folder.
- To build run the build/build.bat
- To run call the generated main.exe

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

Config: (source/engine.h CONFIG ZONE)

- char APP_NAME[] = "WorkingTitle";
- int DISPLAY_WIDTH = 1280;
- int DISPLAY_HEIGHT = 720;
- bool FULL_SCREEN = false;
- bool PIXEL_ART = false;
- bool SHOW_CURSOR = false;
- bool DEBUG = false;
