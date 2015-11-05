////////////////////////////////////////////////////////////////////////////////
//
// Octet (C) Andy Thomason 2012-2014
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Lasertrap (C) Christian Guckelsberger 2015
// Prototype of art-theft stealth game

#include <string>
#include <sstream>
#include <memory>
#include <stdlib.h>
#include <time.h> 
#include "sprite.h"
#include "character.h"
#include "painting.h"
#include "laser.h"
#include "noisy_texture_shader.h"
#include "fading_texture_shader.h"
namespace octet {

	static const int UNITS_X = 40;
	static const int UNITS_Y = 40;
	static const int TILE_SIZE = 32;

// Prototype of art-theft stealth game
class lasertrap : public app {
	int windowWidth, windowHeight;

	// container for all sprites in the game on different depth levels (starting with floor, then wall shadows, then walls, etc.)
	std::array<std::vector<sprite>, 5> sprites;
	
	// containers for game objects
	std::vector<laser> lasers;
	std::vector<painting> paintings;
	std::vector<sprite> mirrors;

	// game metrics
	int gameStatus = 0;
	int score = 0;
	int nMirrors = 3;

	// using a pointer to the player's character, because there's no default initializer available, but it must be globally accessible
	std::shared_ptr<character> thief;

	// the different shaders used in the game
	texture_shader shader;
	noisy_texture_shader noisy_shader;
	fading_texture_shader fading_shader;

	// a texture and font for our text
	bitmap_font font;
	GLuint font_texture;

	// a texture to be used for mirrors
	GLuint white_texture;

	mat4t cameraToWorld;

	// used to draw an arbitrary text on the screen. Taken from Invaderers example.
	void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
		mat4t modelToWorld;
		modelToWorld.loadIdentity();
		modelToWorld.translate(x, y, 0);
		modelToWorld.scale(scale, scale, 1);
		mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

		enum { max_quads = 32 };
		bitmap_font::vertex vertices[max_quads * 4];
		uint32_t indices[max_quads * 6];
		aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

		unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, font_texture);

		shader.render(modelToProjection, 0);

		glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
		glEnableVertexAttribArray(attribute_pos);
		glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
		glEnableVertexAttribArray(attribute_uv);

		glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
	}

  public:
    /// this is called when we construct the class before everything is initialised.
	lasertrap(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {}

	/// this is called once OpenGL is initialized
    void app_init() {

	  // initialize all shaders
	  shader.init();
	  noisy_shader.init();
	  fading_shader.init();

      cameraToWorld.loadIdentity();
	  cameraToWorld.translate(0, 0, 1);

	  // load globally used textures
	  font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");
	  white_texture = resource_dict::get_texture_handle(GL_RGBA, "#ffffff");

	  // seed random number generator to be used with noisy_texture_shader
	  srand(time(NULL));

	  // load the first level
	  load_level(1);

    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {

		// simulate world - this is mainly about moving the player, placing / collecting mirrors, and collision detection
		simulate();

		// set a viewport - includes whole window area
		glViewport(x, y, w, h);

		// clear the background to black
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
		glDisable(GL_DEPTH_TEST);

		// allow alpha blend (transparency when alpha channel is 0)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// draw floor, shadows, walls
		for (std::vector<sprite> s : sprites) {
			for(sprite sp: s){
				sp.render(shader, cameraToWorld);
			}
		}
		
		// draw lasers with noise using dedicated shader
		for (laser l : lasers) {
			l.render(noisy_shader,cameraToWorld);
		}

		// draw paintings and highlight those which can be collected
		for (painting p : paintings) {
			if (thief->close_to(p)) {
				p.render(fading_shader, cameraToWorld);
			}
			else {
				p.render(shader, cameraToWorld);
			}
		}

		// draw mirrors which can be collected
		for (sprite m : mirrors) {
			m.render(shader, cameraToWorld);
		}

		// draw character
		thief->render(shader, cameraToWorld);

		// draw information on running game
		char score_text[50];
		sprintf(score_text, "Score: %d \nMirrors: %d \nStatus: %d \n", score, nMirrors, gameStatus);
		draw_text(shader, 0.85f, 0.75f, 1.0f / 1024, score_text);
    }

	// simulate world - this is mainly about moving the player, placing / collecting mirrors, and collision detection
	void simulate() {

		if (gameStatus!=0) {
			return;
		}

		//Move character if user pressed a key
		perform_character();

		//Ordinary collision detection. Only update the player character's sprite if there is no collision
		bool collision = false;
		for (sprite &wall : sprites[2]) {
			collision = thief->collides_with(wall);
			if (collision) {
				break;
			}
		}
		if (!collision) {
			for (sprite &obstacle : sprites[3]) {
				collision = thief->collides_with(obstacle);
				if (collision) {
					break;
				}
			}
		}
		thief->update(!collision);

		//Check if mirror is in one of the lasers and adjust them accordingly
		for (laser &l : lasers) {
			for (sprite mirrorSprite : mirrors) {
				l.collides(mirrorSprite.get_pos());
			}
		}

		//Check if character is in one of the lasers and adjust laser accordingly / trigger alarm
		for (laser &l : lasers) {
			if (l.collides(thief->get_pos())) {
				gameStatus = -1;
			}
		}

	}

	//Character actions: 0=rotate counterclockwise, 1=rotate clockwise, 2=move forward, 3=move backward
	void perform_character() {
		const float walking_speed = 0.05f;
		// left and right arrows: rotation
		if (is_key_down(key_left)) {
			thief->move(0);
		}
		else if (is_key_down(key_right)) {
			thief->move(1);
		}
		// up and down arrows: walking
		else if (is_key_down(key_up)) {
			thief->move(2);
		}
		else if (is_key_down(key_down)) {
			thief->move(3);
		}
		// if thief is close enough to a painting to grab it, get it.
		else if (is_key_going_up(key_space)) {

			//Check if close to painting: pick it up
			for (int i = 0; i < paintings.size(); i++) {
				if (thief->close_to(paintings[i])) {
					paintings.erase(paintings.begin() + i);

					// If ordinary painting, increase player score by a small amount of credits.
					// If it's the goal painting, increase score massively and set game status to won.
					if (paintings[i].getIsMainObjective()) {
						score += 10000;
						gameStatus = 1;
					}
					else {
						score += (rand() % (int)1001);
					}
					return;
				}
			}

			//Check if close to mirror: pick it up
			for (int i = 0; i < mirrors.size(); i++) {
				if (thief->collides_with(mirrors[i])) {
					mirrors.erase(mirrors.begin() + i);
					++nMirrors;

					for (laser &l : lasers) {
						l.reset();
					}
					return;
				}
			}

			//Else, place mirror if there are still mirrors available
			if(nMirrors>0){

				sprite mirrorSprite;
				float spriteSize = 1.0f / std::min((windowWidth / UNITS_X), (windowHeight / UNITS_Y));
				vec2 thiefPosition = thief->get_pos();
				int thiefRotation = thief->get_rot();
				float rad = thiefRotation*(3.14159265f / 180);
				thiefPosition[0]= thiefPosition[0] + cosf(rad)*0.1f;
				thiefPosition[1] = thiefPosition[1] + sinf(rad)*0.1f;

				mirrorSprite.init(white_texture, 0, 0, 0, spriteSize, spriteSize);
				mirrorSprite.set_pos(thiefPosition);
				mirrors.push_back(mirrorSprite);

				--nMirrors;
			}
		}
	}

	// load level with level number from .csv files
	void load_level(int n) {

		//Adjust sprite size to fill viewport on shortest side
		get_viewport_size(windowWidth, windowHeight);
		float spriteSize = 1.0f / std::min((windowWidth / UNITS_X), (windowHeight / UNITS_Y));

		//Determine working directory in order to use relative paths
		char buf[256];
		getcwd(buf, sizeof(buf));
		sprintf(buf, "%s%s%d", buf, "\\resources\\level", n);

		//Create paths to csv files for each layer
		string path(buf);
		std::array<string, 6> layerPaths{ path,path,path,path,path,path};
		layerPaths[0] += "\\museum_floor.csv";
		layerPaths[1] += "\\museum_walls_shadows.csv";
		layerPaths[2] += "\\museum_walls.csv";
		layerPaths[3] += "\\museum_obstacles.csv";
		layerPaths[4] += "\\museum_lasers.csv";
		layerPaths[5] += "\\museum_paintings.csv";
		string tilesheetPath = path;
		tilesheetPath+="\\tilesheet.tga";

		//Read csv files with level definitions, and initialise sprites (floor, wall shadows, walls, obstacles such as couches)
		std::array<int, 1600> world;
		GLuint tilesheetId = resource_dict::get_texture_handle(GL_RGBA, tilesheetPath);
		int tilesheet_size = 512;
		for (int i = 0; i <=4 ; i++) {
			readCsv(layerPaths[i], world);
			init_sprites(tilesheetId, tilesheet_size, world, spriteSize, sprites[i]);
		}

		// initialise lasers separetely, because they will be initialised as dedicated objects
		init_lasers(world, spriteSize);

		// initialise paintings separetely, because they will be initialised as dedicated objects
		std::array<int, 1600> goalPaintings;
		readCsv(layerPaths[5], world);
		init_paintings(tilesheetId, tilesheet_size, world, spriteSize);
		paintings.back().setIsMainObjective(true);

		// load player character
		path += "\\player.gif";
		GLuint tex = resource_dict::get_texture_handle(GL_RGBA, path);
		sprite thiefSprite;
		thiefSprite.init(tex, 1, 4, 0, spriteSize*1.5, spriteSize*1.5);
		thief = std::make_shared<character>();
		thief->init(thiefSprite);

	}

	//Read csv file with max. 1600 integer entries and store them in an array
	void readCsv(const char *path, std::array<int, 1600> &values) {
		std::ifstream fs(path);
		std::istringstream ls;
		std::string line;
		std::string value;

		//For each row (line)
		int i = 0;
		while (fs.good())
		{
			std::getline(fs, line, '\n');
			if (line.empty()) break;

			//For each column
			ls.str(line);
			while (ls.good()) {
				std::getline(ls, value, ',');
				int spriteId = std::stoi(value);
				values[i] = spriteId;
				++i;
			}
			ls.clear();
		}
	}

	//Initialize paintings, i.e. summarize vertical/horizontal row of sprites into one object
	void init_paintings(const GLuint tilesheetId, const int tilesheet_size, std::array<int, 1600> &ordinaryPaintings, const float spriteSize) {
		int posX, posY;
		for (int id = 0; id < ordinaryPaintings.size(); ++id) {
			if (ordinaryPaintings[id] > -1) {
				std::vector<sprite> sprites;
				//Collect horizontal painting sprites
				if (ordinaryPaintings[id + 1] > -1) {
					while (ordinaryPaintings[id] > -1) {
						int texId = ordinaryPaintings[id];
						ordinaryPaintings[id] = -1;
						int posX = (id%UNITS_X);
						int posY = id / UNITS_Y;
						sprites.push_back(sprite());
						sprites.back().init(tilesheetId, posX, posY, 0, spriteSize, spriteSize, texId, TILE_SIZE, TILE_SIZE, tilesheet_size, tilesheet_size);
						++id;
					}
				}
				//Collect vertical painting sprites
				else if (ordinaryPaintings[id + UNITS_X] > -1) {
					int fwId = id;
					while (ordinaryPaintings[fwId] > -1) {
						int texId = ordinaryPaintings[fwId];
						ordinaryPaintings[fwId] = -1;
						int posX = (fwId%UNITS_X);
						int posY = fwId / UNITS_Y;
						sprites.push_back(sprite());
						sprites.back().init(tilesheetId, posX, posY, 0, spriteSize, spriteSize, texId, TILE_SIZE, TILE_SIZE, tilesheet_size, tilesheet_size);
						fwId+=UNITS_X;
					}
				}

				//Assign them to one painting object
				paintings.push_back(painting(sprites, false));

			}

		}
	}

	// load lasers into dedicated objects.
	// Load lasers. Different directions indicated by different type numbers in the world.
	// 108: laser up -> low
	// 123: laser low -> up
	// 124: laser right -> left
	// 107: laser left -> right
	void init_lasers(const std::array<int, 1600> &values, const float spriteSize) {
		
		// red texture for the laser
		GLuint texture = resource_dict::get_texture_handle(GL_RGBA, "#ff0000");

		// distinguish between lasers pointing into different directions and initialise the object accordingly
		int posX, posY;
		for (int id = 0; id < values.size(); ++id) {
			int type = values[id];
			posX = (id%UNITS_X);
			posY = id / UNITS_Y;
			switch (type) {
			case 108:
				lasers.push_back(laser(texture, vec2(posX, posY), vec2(0, -1), spriteSize, sprites[2], mirrors));
				break;
			case 123:
				lasers.push_back(laser(texture, vec2(posX, posY), vec2(0, 1), spriteSize, sprites[2], mirrors));
				break;
			case 124:
				lasers.push_back(laser(texture, vec2(posX, posY), vec2(-1, 0), spriteSize, sprites[2], mirrors));
				break;
			case 107:
				lasers.push_back(laser(texture, vec2(posX, posY), vec2(1, 0), spriteSize, sprites[2], mirrors));
				break;
			}
		}
	}

	//Create sprite for every non-empty part of the level. Iterate over different layers of the level definition.
	void init_sprites(const GLuint tilesheetId, const int tilesheet_size, const std::array<int, 1600> &values, const float spriteSize, std::vector<sprite> &sprites) {

		for (int layer = 0; layer < 3; ++layer) {
			for (int id = 0; id < values.size(); ++id) {
				int texId = values[id];

				//Only create sprites for non-black areas
				if (texId < 0) continue;

				//vec2 positions = posFromId(id, spriteSize);
				int posX = (id%UNITS_X);
				int posY = id / UNITS_Y;

				sprites.push_back(sprite());
				sprites.back().init(tilesheetId, posX, posY, 0, spriteSize, spriteSize, texId, TILE_SIZE, TILE_SIZE, tilesheet_size, tilesheet_size);
			}
		}
	}

  };
}

