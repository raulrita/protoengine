#include "external/engine.h"

Texture hat;

void game_init()
{	
	hat = load_texture("res/hat.png");
	hat.position.x = 0;
	hat.position.y = 0;
}

void game_tick(const float delta)
{
	draw(hat);
}

void game_terminate()
{
	unload_texture(hat);
}
