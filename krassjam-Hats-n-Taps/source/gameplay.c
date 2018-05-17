/***************
/* gameplay
***************/

#define TILE_TEXTURES_COUNT 10
#define MAP_SIZE 35

const int COLUMNS = 7;
const int ROWS = 5;
const int TILE_MARGIN = 8;
const int OFFSET = 24;
const float MOVE_TIME = 250; // 250 miliseconds
const int SIDE = 200;
const float SWAP_TILES_TIME = 250;
const int PLAYER_OFFSET = 33;
const int PLAYER_SHADOW_OFFSET_X = -12;
const int PLAYER_SHADOW_OFFSET_Y = 100;
const float PLAYER_MAX_TILT = 20.f;
const float PLAYER_TILT = 1.5f;
const float PLAYER_ROTATION = 1.5f;
const int PIN_OFFSET_X = 40;
const int PIN_OFFSET_Y = 30;
const int PIN_SHADOW_OFFSET_X = -20;
const int PIN_SHADOW_OFFSET_Y = 110;
const int HAT_SHADOW_OFFSET_X = 22;
const int HAT_SHADOW_OFFSET_Y = 120;
const int HAT_OFFSET_X = 20;
const int HAT_OFFSET_Y = 33;

float pick_time;
bool pick_show;
bool pick_show_master;

byte map[MAP_SIZE];
bool hats[MAP_SIZE];
float swap_time;
int hat_count = 0;

Texture tile_textures[TILE_TEXTURES_COUNT];
Texture shadow;
Texture pin;
Texture hat;
Texture pick;

Vector end;

typedef struct Player
{
	Texture texture;
	Vector start;
	Vector target;

	Vector position; // in board coords
	Vector position_target; //in board coords
	
	bool moving;
	bool flipped;
	
	float time;

	bool tilt_forward;
	float tilt;

	float rotation;
	int last_direction; // for sliding
} Player;

Player player;

Vector pos(const int x, const int y)
{
	Vector result;

	result.x = OFFSET + x * (SIDE + TILE_MARGIN);
	result.y = OFFSET + y * (SIDE + TILE_MARGIN);
	
	return result;
}

void level_over()
{
	if (config.level < MAX_LEVELS)
	{		
		config.level++;
	
		if (config.max < config.level)
			config.max = config.level;

		over_mode = 1;
		current_scene = 4;
	}
	else
	{
		over_mode = 2;
		current_scene = 4;
		config.level = 1;
	}
}

void draw_tile(const byte tile_number,
			   const int pos_x,
			   const int pos_y)
{
	Texture tex = tile_textures[tile_number - 1];
	tex.position = pos(pos_x, pos_y);
   	
	draw(tex);

	//log("%i", tex.height);
	//log("here %f %f", tex.position.x, tex.position.y);
}

void draw_hat( const int pos_x, const int pos_y)
{
	hat.position = pos(pos_x, pos_y);

	shadow.position = hat.position;
	shadow.position.x += HAT_SHADOW_OFFSET_X;
	shadow.position.y += HAT_SHADOW_OFFSET_Y;

	hat.position.x += HAT_OFFSET_X;
	hat.position.y += HAT_OFFSET_Y;

	hat.position.x += cos(player.rotation) * HALF_PI / 2.f;
	hat.position.y -= player.tilt / 2.f;
	
	draw(shadow);
	draw(hat);
}

void draw_player()
{
	shadow.position = player.texture.position;
	shadow.position.x += PLAYER_SHADOW_OFFSET_X;
	shadow.position.y += PLAYER_SHADOW_OFFSET_Y;

	// save to apply tilt
	Vector aux = player.texture.position;
	
	player.texture.position.x += cos(player.rotation) * HALF_PI;
	player.texture.position.y -= player.tilt;   
		
	draw(shadow);
	draw(player.texture);

	player.texture.position = aux;
}

void player_moving(const float delta)
{	
	player.time += delta * FRAME_TARGET;

	float percentage = player.time / MOVE_TIME;

	if (percentage < 1.f)
	{
		player.texture.position.x = lerp(
			player.start.x,
			player.target.x,
			percentage);

		player.texture.position.y = lerp(
			player.start.y,
			player.target.y,
			percentage);

		player.texture.rotation =
			percentage * 360 *
			(player.flipped ? -1.f : 1.f);
	}
	else
	{
		player.texture.position = player.target;
		player.texture.rotation = 0.f;
		player.moving = false;
		player.time = 0.f;
		player.position = player.position_target;
	}
}

bool can_player_move(const Vector target)
{
	if (target.y < 0 || target.y > ROWS - 1)
		return false;

	if (target.x < 0 || target.x > COLUMNS - 1)
		return false;
	
	int pos = target.y * COLUMNS + target.x;
	
	if (map[pos] > 0 && map[pos] != 4)
		return true;

	return false;
}

void player_move(const int direction)
{
	Vector new_pos = player.position;

	// CLOCKWISE STARTING AT LEFT
	switch (direction)
	{
	case 0: new_pos.x--; break; // LEFT
	case 1: new_pos.y--; break; // UP
	case 2: new_pos.x++; break; // RIGHT
	case 3: new_pos.y++; break; // DOWN
	}

	// check if player can move there
	if (! can_player_move(new_pos))
		return;

	int map_pos = player.position.y * COLUMNS + player.position.x;
	if (map[map_pos] == 10)
		map[map_pos] = 4; // turn red

	player.position_target = new_pos;	
	player.last_direction = direction;
	player.flipped = direction == 1 || direction == 2;
	player.moving = true;
	player.time = 0.f;
	player.start = player.texture.position;
	player.target = pos(
		player.position_target.x,
		player.position_target.y);

	player.target.x += PLAYER_OFFSET;
	player.target.y += PLAYER_OFFSET;	
}

void player_logic(const float delta)
{
	// tilt
	if (player.tilt_forward && player.tilt > PLAYER_MAX_TILT)
		player.tilt_forward = !player.tilt_forward;

	else if (!player.tilt_forward && player.tilt < 0)
		player.tilt_forward = !player.tilt_forward;
	
	player.tilt += delta * PLAYER_TILT *
		(player.tilt_forward ? 1.f : -1.f);

	// rotation
	player.rotation += delta * PLAYER_ROTATION;
	
	if (player.moving)
	{
		player_moving(delta);
		return;
	}
	
	// check for ice
	int pos = player.position.y * COLUMNS + player.position.x;
	if (map[pos] == 5)
	{
		player_move(player.last_direction);
		return;
	}

    // check for movers
	if (map[pos] >= 6 && map[pos] <= 9)
	{
		// tile from 6 till 9 have
		// the same order as direction
		player_move(map[pos] - 6);
		return;
	}

	// check for hats
	if (hats[pos])
	{
		hat_count--;
		hats[pos] = false;
	}
	
	// check if at the end
	if (player.position.x == end.x && player.position.y == end.y)
	{
		if (hat_count == 0)
			level_over();
		else
			pick_show_master = true;
	}
	
	if (key_up(VK_LEFT))
		player_move(0);
	else if (key_up(VK_UP))
		player_move(1);
	else if (key_up(VK_RIGHT))
		player_move(2);
	else if (key_up(VK_DOWN))
		player_move(3);
}

void load_level()
{	
	int offset = (config.level - 1) * MAP_SIZE;
	byte value;
	hat_count = 0;
	
	for (int h = 0; h < MAP_SIZE; h++)
	{
		// TILES
		value = LEVELS[offset + h];

		switch (value)
		{
		case 1:
		{
			int rnd = rand() % 100;
			if (rnd > 85)
				value = 3;
			else if (rnd > 70)
				value = 2;
		}
		break;
		
		}
		
		map[h] = value;

		// HATS
		value = HATS[offset + h];	   
		hats[h] = value;

		if (value == 1)
			hat_count++;
	}
}

void load_tile_textures()
{
	char filename[50];

	for (int h = 0; h < TILE_TEXTURES_COUNT; h++)
	{
		sprintf(filename, "res/tile%i.png", h + 1);
		tile_textures[h] = load_texture(filename);
	}	
}

void load_player()
{
	player.moving = false;
	player.rotation = 0.f;
	player.tilt = 0.f;
	player.tilt_forward = false;
	player.texture = load_texture("res/player.png");

	int offset = (config.level - 1) * 2; // x,y

	player.position.x = STARTS[offset];
	player.position.y = STARTS[offset + 1];

	player.texture.position = pos(
		player.position.x,
		player.position.y);

	player.texture.position.x += PLAYER_OFFSET;
	player.texture.position.y += PLAYER_OFFSET;
	
	player.start = player.target =
		player.texture.position;
}

void swap_tiles(const float delta)
{
	swap_time += delta * FRAME_TARGET;

	if (swap_time > SWAP_TILES_TIME)
	{
		swap_time = 0.f;

		for (int h = 0; h < MAP_SIZE; h++)
		{
			byte value = map[h];

			if (value >= 1 && value <= 3)
			{
				int rnd = rand() % 100;

				if (rnd > 85)
					value = 3;
				else if (rnd > 70)
					value = 2;
				else
					value = 1;

				map[h] = value;
			}
		}		
	}
}

void load_pin()
{
	pin = load_texture("res/pin.png");

	int offset = (config.level - 1) * 2; // x,y

	end.x = PINS[offset];
	end.y = PINS[offset + 1];

	pin.position = pos(
		end.x,
		end.y);

	pin.position.x += PIN_OFFSET_X;
	pin.position.y += PIN_OFFSET_Y;
}

void draw_pin()
{
	shadow.position = pin.position;
	shadow.position.x += PIN_SHADOW_OFFSET_X;
	shadow.position.y += PIN_SHADOW_OFFSET_Y;

	// save to apply tilt
	Vector aux = pin.position;

	pin.position.x -= cos(player.rotation) * HALF_PI / 2.f;
	pin.position.y += player.tilt / 2.f;

	draw(shadow);
	draw(pin);

	pin.position = aux;
}

void pick_tick(const float delta)
{
	pick_time += delta * FRAME_TARGET;
	if (pick_time > BLINK_TIME)
	{
		pick_show = !pick_show;
		pick_time = 0.f;
	}
}

void load_pick()
{
	pick = load_texture("res/pick.png");
	pick.position.x = 1500;
	pick.position.y = 380;

	pick_show = false;
	// only show when player hits end pos and has not picked up all hats
	pick_show_master = false;
}

void gameplay_init()
{
	swap_time = 0;
	hat_count = 0;
    load_tile_textures();
	load_player();
	load_level();
	load_pin();
	load_pick();
	
	shadow = load_texture("res/shadow.png");
	hat = load_texture("res/hat.png");
}

void gameplay_tick(const float delta)
{
	if (key_up(VkKeyScan('r')))
		current_scene = current_scene == 1 ? 2 : 1;

	if (key_up(VkKeyScan('e')))
		quit = true;

	if (key_up(VkKeyScan('p')) && config.level > 1)
	{
		config.level--;
		current_scene = current_scene == 1 ? 2 : 1;
	}

	if (key_up(VkKeyScan('n')) && config.level < config.max)
	{
		config.level++;
		current_scene = current_scene == 1 ? 2 : 1;
	}

	if (key_up(VkKeyScan('h')))
	{
		over_mode = 3;
		current_scene = 4;		
	}
	
	
	// UPDATE
	player_logic(delta);

	// swap tiles
	swap_tiles(delta);

	// blink pick
	pick_tick(delta);
	
	// DRAW
	byte index;
	for (int y = 0; y < ROWS; y++)
	{
		for (int x = 0; x < COLUMNS; x++)
		{			
			index = map[y * COLUMNS + x];

			if (index > 0)
				draw_tile(index, x, y);

			index = hats[y * COLUMNS + x];

			if (index > 0)
				draw_hat(x, y);			
		}
	}

	draw_pin();
	draw_player();

	if (pick_show_master && pick_show)
		draw(pick);
	
	//log("gameplay tick");
}

void gameplay_terminate()
{
	for (int h = 0; h < TILE_TEXTURES_COUNT; h++)
		unload_texture(tile_textures[h]);

	unload_texture(pick);
	unload_texture(hat);
	unload_texture(pin);
	unload_texture(player.texture);
	unload_texture(shadow);
}
