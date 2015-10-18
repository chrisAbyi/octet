////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

#include<string>
#include<sstream>
#include<memory>
#include "sprite.h"
#include "character.h"

namespace octet {

	static const int UNITS_X = 40;
	static const int UNITS_Y = 40;
	static const int TILE_SIZE = 32;
	
	class laser {
		const vec2 positionOrigin;
		const vec2 direction;
		const std::array<int, 1600> &world;
		vec2 positionHit;
		float unitSize;
	public:

		laser(const vec2 _positionOrigin, const vec2 _direction, const float _unitSize, const std::array<int, 1600> &_world) : positionOrigin(_positionOrigin), direction(_direction), unitSize(_unitSize), world(_world) {
			checkHit();
		}

		void checkHit() {
			positionHit = positionOrigin;
			int id = positionHit[1] * UNITS_X + positionHit[0];

			while (world[id]!=0 && world[id]!=1) {
				positionHit += direction;
				id = positionHit[1] * UNITS_X + positionHit[0];
			}
		}

		void render() {
			
			float t = (1 - unitSize / 2);

			float xOrigin = (positionOrigin[0]*unitSize) - t;
			float yOrigin = (positionOrigin[1]*unitSize) - t;
			yOrigin *= -1;

			float xHit = (positionHit[0] * unitSize) - t;
			float yHit = (positionHit[1] * unitSize) - t;;
			yHit *= -1;

			glLineWidth(2.5);
			glColor3f(1.0, 0.0, 0.0);
			glBegin(GL_LINES);
			glVertex3f(xOrigin, yOrigin, 0.0);
			glVertex3f(xHit, yHit, 0);
			glEnd();
			
		}
	};

  /// Scene containing a box with octet.
  class lasertrap : public app {
	int windowWidth, windowHeight;
	int gameStatus = 0;
	std::array<std::vector<sprite>, 4> sprites;
	std::array<int, 1600> world;
	std::vector<laser> lasers;
	std::shared_ptr<character> thief;

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
		simulate();
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
		
		for (laser l : lasers) {
			l.render();
		}

		thief->render(shader, cameraToWorld);
		

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

	// called every frame to move things
	void simulate() {
		if (gameStatus!=0) {
			return;
		}

		move_character();
	}

	void move_character() {
		const float walking_speed = 0.05f;
		// left and right arrows: rotation
		if (is_key_down(key_left)) {
			thief->rotateLeft();
		}
		else if (is_key_down(key_right)) {
			thief->rotateRight();
		}
		// up and down arrows: walking
		else if (is_key_down(key_up)) {
			thief->moveForward();
		}
		else if (is_key_down(key_down)) {
			thief->moveBackward();
		}
	}

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
		std::array<string, 4> layerPaths{ path,path,path,path};

		layerPaths[0] += "\\museum_floor.csv";
		layerPaths[1] += "\\museum_walls.csv";
		layerPaths[2] += "\\museum_items.csv";
		layerPaths[3] += "\\world.csv";
		path += "\\tilesheet.gif";

		//Read csv files with level definitions, and initialise sprites
		for (int i = 0; i < 3; i++) {
			readCsv(layerPaths[i], world);
			init_sprites(path, world, spriteSize, sprites[i]);
			std::array<int, 1600>().swap(world);
		}

		//Read csv file with world definition: abstraction of elements in the world e.g. to collision objects, goal objects, etc.
		readCsv(layerPaths[3], world);

		/*
		Load lasers. Different directions indicated by different type numbers in the world.
		5: laser up -> low
		6: laser low -> up
		7: laser right -> left
		8: laser left -> right
		*/
		int posX, posY;
		for (int id = 0; id < world.size(); ++id){
			int type = world[id];
			if (type > 4 && type < 9) {
				posX = (id%UNITS_X);
				posY = id / UNITS_Y;
				switch (type) {
				case 5:
					lasers.push_back(laser(vec2(posX, posY), vec2(0, 1), spriteSize, world));
					break;
				case 6:
					lasers.push_back(laser(vec2(posX, posY), vec2(0, -1), spriteSize, world));
					break;
				case 7:
					lasers.push_back(laser(vec2(posX, posY), vec2(-1, 0), spriteSize, world));
					break;
				case 8:
					lasers.push_back(laser(vec2(posX, posY), vec2(1, 0), spriteSize, world));
					break;
				}
			}
		}

		GLuint tex = resource_dict::get_texture_handle(GL_RGBA, "#ffffff");
		sprite thiefSprite;
		thiefSprite.init(tex, 1, 7, 0, spriteSize, spriteSize);
		thief = std::make_shared<character>(world);
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

	//Create sprite for every non-empty part of the level. Iterate over different layers of the level definition.
	void init_sprites(const char *pathTilesheet, const std::array<int, 1600> &values, const float spriteSize, std::vector<sprite> &sprites) {

		GLuint tilesheet = resource_dict::get_texture_handle(GL_RGBA, pathTilesheet);
		int tilesheet_size = 512;

		for (int layer = 0; layer < 3; ++layer) {
			for (int id = 0; id < values.size(); ++id) {
				int texId = values[id];

				//Only create sprites for non-black areas
				if (texId < 0) continue;

				//vec2 positions = posFromId(id, spriteSize);
				int posX = (id%UNITS_X);
				int posY = id / UNITS_Y;

				sprites.push_back(sprite());
				sprites.back().init(tilesheet, posX, posY, 0, spriteSize, spriteSize, texId, TILE_SIZE, TILE_SIZE, tilesheet_size, tilesheet_size);
			}
		}
	}
  };
}

