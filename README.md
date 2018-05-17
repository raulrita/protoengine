# protoengine

Proto Engine aims to be a simple self contained 2d custom game engine. 
The main goal is to provide a simple no fuzz prototyping tool in c.

Written in c and opengl using the awesome tcc compiler (Tiny C compiler).

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
Installation:

Simply download the repository and place in c:\proto. You're good to go.

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

To create new Prototype:

1. Copy paste the template folder and rename to anything you want
2. The entire source is in source/external and the actual game code should in the source
3. Start messing with source\main.c
4. Place any image files (png 32bit) in build/res folder.
5. To build run the build/build.bat
6. To run call the generated main.exe

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

Config: (source/engine.h CONFIG ZONE)
char APP_NAME[] = "WorkingTitle";
int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
bool FULL_SCREEN = false;
bool PIXEL_ART = false;
bool SHOW_CURSOR = false;
bool DEBUG = false;
