
//Texture over_background;
Texture over_text;
Texture over_any;

float over_time;
bool over_show_any;

void over_init()
{
	switch (over_mode)
	{
	case 1: over_text = load_texture("res/levelup.png"); break;
	case 2: over_text = load_texture("res/gameover.png"); break;
	case 3: over_text = load_texture("res/help.png"); break;
	}

	over_text.position.x =
		DISPLAY_WIDTH / 2 - 200 -
		over_text.width / 2;

	over_text.position.y =
		DISPLAY_HEIGHT / 2 -
		over_text.height / 2;

	over_any = load_texture("res/anykey.png");

	over_any.position.x = 35;
	over_any.position.y = 1027;

	over_time = 0.f;
	over_show_any = false;
}

void over_tick(const float delta)
{
	if (key_any)
		current_scene = 1;

	over_time += delta * FRAME_TARGET;
	if (over_time > BLINK_TIME)
	{
		over_show_any = !over_show_any;
		over_time = 0.f;
	}
		
	draw(over_text);

	if (over_show_any)
		draw(over_any);
}

void over_terminate()
{
	unload_texture(over_any);	
	unload_texture(over_text);
	//unload_texture(over_background);
}
