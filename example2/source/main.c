#include "external/engine.h"

Texture hat;
Texture tile;

void draw_grid()
{
	int offset = 10;
	int margin = 20;

	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			tile.position.x = margin + x * (tile.width + offset);
			tile.position.y = margin + y * (tile.height + offset);

			draw(tile);
		}
	}
}

void game_init()
{	
	hat = load_texture("res/hat.png");
	hat.position.x = 35;
	hat.position.y = 60;

	tile = load_texture("res/tile.png");
}

void game_tick(const float delta)
{
	draw_grid();
	draw(hat);
}

void game_terminate()
{
	unload_texture(tile);
	unload_texture(hat);
}