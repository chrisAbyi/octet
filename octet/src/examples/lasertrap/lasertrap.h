////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

#include<string>
#include<sstream>
#include "sprite.h"

namespace octet {

  /// Scene containing a box with octet.
  class lasertrap : public app {

	static const int UNITS_X = 40;
	static const int UNITS_Y = 40;
	static const int TILE_SIZE = 32;

	int windowWidth, windowHeight;

	std::array<std::vector<sprite>, 4> sprites;
	std::array<int, 1600> world;

	texture_shader shader;
	mat4t cameraToWorld;

  public:
    /// this is called when we construct the class before everything is initialised.
    lasertrap(int argc, char **argv) : app(argc, argv) {
    }

    /// this is called once OpenGL is initialized
    void app_init() {

	  shader.init();

      cameraToWorld.loadIdentity();
	  cameraToWorld.translate(0, 0, 1);

	  load_level(1);
    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {

		//++framecount;
		//player.move((framecount / 4) % 4);

		//int vx = 0, vy = 0;
		//get_viewport_size(vx, vy);

		// set a viewport - includes whole window area
		glViewport(x, y, w, h);

		// clear the background to black
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
		glDisable(GL_DEPTH_TEST);

		// allow alpha blend (transparency when alpha channel is 0)
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0);

		glEnable(GL_BLEND);

		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//read_input();
		//simulate();

		for (std::vector<sprite> s : sprites) {
			for(sprite sp: s){
				sp.render(shader, cameraToWorld);
			}
		}

		/*
      int vx = 0, vy = 0;
      get_viewport_size(vx, vy);
      app_scene->begin_render(vx, vy);

      // update matrices. assume 30 fps.
      app_scene->update(1.0f/30);

      // draw the scene
      app_scene->render((float)vx / vy);

      // tumble the box  (there is only one mesh instance
      scene_node *node = app_scene->get_mesh_instance(0)->get_node();
      node->rotate(1, vec3(1, 0, 0));
      node->rotate(1, vec3(0, 1, 0));*/
    }

	void readCsv(const char *path, std::array<int,1600> &values) {
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

	//Load sprite identifier for every cell on every layer
	void init_sprites(const char *pathTilesheet, const std::array<int,1600> &values, std::vector<sprite> &sprites) {

		GLuint tilesheet = resource_dict::get_texture_handle(GL_RGBA, pathTilesheet);
		int tilesheet_size = 512;
		float spriteSize = 1.0f / std::min((windowWidth / UNITS_X), (windowHeight / UNITS_Y));

		for (int layer = 0; layer < 3; ++layer) {
			for (int id = 0; id < values.size(); ++id) {
				int texId = values[id];

				//Only create sprites for non-black areas
				if (texId < 0) continue;

				vec2 positions = posFromId(id, spriteSize);

				sprites.push_back(sprite());
				sprites.back().init(tilesheet, positions[0], -positions[1], spriteSize, spriteSize, texId, TILE_SIZE, TILE_SIZE, tilesheet_size, tilesheet_size);
			}
		}
	}

	inline vec2 posFromId(int id, float spriteSize) {
		int posX = (id%UNITS_X);
		int posY = id / UNITS_Y;
		float x = (posX * spriteSize) - (1 - spriteSize / 2);
		float y = (posY * spriteSize) - (1 - spriteSize / 2);
		return vec2(x, y);
	}

	/*
	void collision(const vec2 &direction, const float spriteSize, vec2 position) {
		int id = position[1] * UNITS_X + position[0];
		while (world[id] != 0 && world[id] != 4) {
			position += direction;
			id = position[1] * UNITS_X + position[0];
		}
	}*/

	void load_level(int n) {

		get_viewport_size(windowWidth, windowHeight);

		//Determine working directory in order to use relative paths
		char buf[256];
		getcwd(buf, sizeof(buf));
		sprintf(buf, "%s%s%d", buf, "\\resources\\level", n);

		//Create paths to csv files for each layer
		string path(buf);
		std::array<string, 4> layerPaths{ path,path,path,path};

		layerPaths[0] += "\\museum_floor.csv";
		layerPaths[1] += "\\museum_walls.csv";
		layerPaths[2] += "\\museum_items.csv";
		layerPaths[3] += "\\world.csv";
		path += "\\tilesheet.gif";

		for (int i = 0; i < 3; i++) {
			readCsv(layerPaths[i], world);
			init_sprites(path, world, sprites[i]);
			std::array<int, 1600>().swap(world);
		}
		readCsv(layerPaths[3], world);

		/*
		5: laser up -> low
		6: laser low -> up
		7: laser right -> left
		8: laser left -> right
		*/
		
		float spriteSize = 1.0f / std::min((windowWidth / UNITS_X), (windowHeight / UNITS_Y));
		GLuint red = resource_dict::get_texture_handle(GL_RGB, "#ff0000");
		for (int id = 0; id < world.size(); ++id){
			int type = world[id];
			if(type>4 && type<9){
				sprites[3].push_back(sprite());
				//int posX = (id%UNITS_X);
				//int posY = id / UNITS_Y;
				vec2 positions = posFromId(id, spriteSize);

				switch (type) {
				case 5:
					//vec2 endPosition = vec2(posX, posY);
					//collision(vec2(0, 1), spriteSize, endPosition);
//					sprites[3].back().init(red, positions[0], -positions[1], 0.1f, endPosition[1]-posY);
					sprites[3].back().init(red, positions[0], -positions[1], 0.1f, 0.1f);
					break;
				case 6:
					sprites[3].back().init(red, positions[0], -positions[1], 0.1f, 0.1f);
					break;
				case 7:
					sprites[3].back().init(red, positions[0], -positions[1], 0.1f, 0.1f);
					break;
				case 8:
					sprites[3].back().init(red, positions[0], -positions[1], 0.1f, 0.1f);
					break;
				}
			}
		}

		/*
		0: wall
		1: door
		4: objects
		5: main objective
		6: other objectives
		*/

		//Initialise sprite with reference to spritesheet for every identifier > 0 (i.e. actual tiles in the game)
	}
  };
}

