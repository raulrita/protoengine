#include "external/engine.h"

typedef struct DataConfig
{
	int level;
	int max;
} DataConfig;

const float BLINK_TIME = 500.f;
const int START_SCENE = 3;
const int MAX_LEVELS = 7;

DataConfig config;
byte scene = 0;
byte current_scene = 0;
byte over_mode;

#include "intro.c"
#include "levels.c"
#include "gameplay.c"
#include "over.c"

Texture title;
Texture menu;
Texture patch;

const Vector previous_pos = { 1500, 650 };
const Vector next_pos = { 1500, 740 };

void change_scene()
{
	// first end the current scene
	switch (scene)
	{
	case 1:
	case 2:
		gameplay_terminate(); break;

	case 3: intro_terminate(); break;
	case 4: over_terminate(); break;
	}
	
	switch (current_scene)
	{
	case 1:
	case 2:
		gameplay_init();
		break;

	case 3: intro_init();
	case 4: over_init();
	}

	scene = current_scene;
}

void load_config()
{
	FILE* file = fopen("res/settings.dat", "r");

	if (file != NULL)
		fread(&config, sizeof(config), 1, file);

	fclose(file);
}

void save_config()
{
	FILE* file = fopen("res/settings.dat", "w+");

	if (file != NULL)
		fwrite(&config, sizeof(config), 1, file);

	fclose(file);	
}

void game_init()
{
	// config defaults
	config.level = 1;
	config.max = 1;

	load_config();
	
	current_scene = START_SCENE;

	title = load_texture("res/title.png");
	menu = load_texture("res/menu.png");
	patch = load_texture("res/patch.png");

	title.position.x = 1500;
	title.position.y = 40;
	
	menu.position.x = 1500;
	menu.position.y = 665;
}

void game_tick(const float delta)
{
	if (current_scene != scene)
		change_scene();

	switch (current_scene)
	{
	case 1:
	case 2:
		gameplay_tick(delta);
		break;

	case 3: intro_tick(delta); break;
	case 4: over_tick(delta); break;
	}

	if (current_scene > 2 && current_scene != 4)
		return;
	
	draw(title);
	draw(menu);

	// draw patches if necessary
	if (config.level <= 1)
	{
		patch.position = previous_pos;
		draw(patch);
	}

	if (config.level == config.max)
	{
		patch.position = next_pos;
		draw(patch);		
	}   	
}

void game_terminate()
{
	save_config();
	
	current_scene = 0;
	change_scene(); // terminate current

	unload_texture(patch);
	unload_texture(menu);
	unload_texture(title);
}
