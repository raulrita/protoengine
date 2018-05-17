
const float INTRO_SHOW = 500;
const float INTRO_HIDE = 2000;
const float INTRO_MOVE = 2500;

Texture intro_title;
float intro_time;

void intro_init()
{
	intro_title = load_texture("res/title.png");
	intro_time = 0.f;

	intro_title.position.x =
		DISPLAY_WIDTH / 2 -
		intro_title.width / 2;

	intro_title.position.y =
		DISPLAY_HEIGHT / 2 -
		intro_title.height / 2;
}

void intro_tick(const float delta)
{
	intro_time += delta * FRAME_TARGET;

	if (intro_time > INTRO_SHOW && intro_time < INTRO_HIDE)
		draw(intro_title);

	if (intro_time > INTRO_MOVE)
		current_scene = 1;
}

void intro_terminate()
{
	unload_texture(intro_title);
}
