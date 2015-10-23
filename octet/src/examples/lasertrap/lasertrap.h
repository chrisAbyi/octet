////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

#include<string>
#include<sstream>
#include<memory>
#include <stdlib.h>
#include <time.h> 
#include "sprite.h"
#include "character.h"
#include "noisy_texture_shader.h"
namespace octet {

	static const int UNITS_X = 40;
	static const int UNITS_Y = 40;
	static const int TILE_SIZE = 32;
	
	class laser {
		vec2 p0, p1, p2, p3;
		vec2 origin, hit, wallHit;
		mat4t modelToWorld;
		const std::array<std::vector<sprite>, 4> &sprites;
		float unitSize;
		GLuint texture;
		const vec2 direction;

		void calcWallHit() {

			//Check where laser hits a wall with respect to its orientation
			wallHit = origin;
			boolean collided = false;
			do {
				wallHit += direction*unitSize;
				for (sprite s : sprites[2]) {

					if (isSame(s.get_pos(), wallHit)) {
						collided = true;
						break;
					}
				}
			} while (!collided);

		}

		void calcRectangle() {
			//Offset (must not start in middle, but at beginning of tile)
			p0 = origin - (direction / 2)*unitSize;
			p1 = hit - (direction / 2)*unitSize;

			//Calculate other corners of laser rectangle
			p2 = p1;
			p3 = p0;
			float half_width = 0.004;
			if (p0[0] == p1[0]) {
				p0[0] -= half_width;
				p1[0] -= half_width;
				p2[0] += half_width;
				p3[0] += half_width;
			}
			else {
				p0[1] -= half_width;
				p1[1] -= half_width;
				p2[1] += half_width;
				p3[1] += half_width;
			}
		}

		inline bool isSame(const vec2 v0, const vec2 v1) const {
			if (fabs(v0[0] - v1[0]) < 0.001) {
				if (fabs(v0[1] - v1[1]) < 0.001) {
					return true;
				}
			}
			return false;
		}

	public:
		laser(const vec2 _positionOrigin, const vec2 _direction, const float _unitSize, const std::array<std::vector<sprite>, 4> &_sprites) : unitSize(_unitSize), sprites(_sprites), direction(_direction) {
			//Assign red texture (could be done once for all instances)
			texture = resource_dict::get_texture_handle(GL_RGBA, "#ff0000");

			//Transform abstract coordinates to model-view coordinates
			origin = (_positionOrigin * unitSize) - (1 - unitSize / 2);
			origin[1] *= -1;

			//Set-up wall coordinates and assign them to laser
			calcWallHit();
			hit = wallHit;
			calcRectangle();
		}

		bool collides(const character &_character) {

			//Check where laser hits sprite with respect to its orientation
			const vec2 cPos = _character.get_pos();

			//Outer conditional checks whether sprite close to laser x/y-axis. Inner conditional checks whether sprite between origin and hit point of laser.
			if (fabs(cPos[0] - origin[0]) < unitSize/2 && fabs(cPos[0] - wallHit[0]) < unitSize/2) {
				if ((fabs(cPos[1] - origin[1]) < fabs(origin[1] - wallHit[1])) && (fabs(cPos[1] - wallHit[1]) < fabs(origin[1] - wallHit[1]))) {
					hit = vec2(wallHit[0], cPos[1]);
					calcRectangle();
					return true;
				}
			}
			else if (fabs(cPos[1] - origin[1]) < unitSize/2 && fabs(cPos[1] - wallHit[1]) < unitSize/2) {
				if ((fabs(cPos[0] - origin[0]) < fabs(origin[0] - wallHit[0])) && (fabs(cPos[0] - wallHit[0]) < fabs(origin[0] - wallHit[0]))) {
					hit = vec2(cPos[0], wallHit[1]);
					calcRectangle();
					return true;
				}
			}

			//If sprite not in line (anymore), switch back to wall
			if (!isSame(hit, wallHit)) {
				hit = wallHit;
				calcRectangle();
			}

			return false;
		}

		void render(noisy_texture_shader &shader, mat4t &cameraToWorld) {

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

			// use "old skool" rendering
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0);

			// array with laser ray positions (four corners of rectangle)
			float vertices[] = {
				p0[0], p0[1], 0,
				p1[0], p1[1], 0,
				p2[0], p2[1], 0,
				p3[0], p3[1], 0
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

	};

  /// Scene containing a box with octet.
  class lasertrap : public app {
	int windowWidth, windowHeight;
	int gameStatus = 0;
	std::array<std::vector<sprite>, 4> sprites;
	std::vector<laser> lasers;
	std::shared_ptr<character> thief;

	texture_shader shader;
	noisy_texture_shader noisy_shader;

	mat4t cameraToWorld;

  public:
    /// this is called when we construct the class before everything is initialised.
    lasertrap(int argc, char **argv) : app(argc, argv) {
    }

    /// this is called once OpenGL is initialized
    void app_init() {

	  shader.init();
	  noisy_shader.init();

      cameraToWorld.loadIdentity();
	  cameraToWorld.translate(0, 0, 1);

	  load_level(1);

	  srand(time(NULL));

    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
		simulate();
		//++framecount;
		//player.move((framecount / 4) % 4);

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
		
		for (std::vector<sprite> s : sprites) {
			for(sprite sp: s){
				sp.render(shader, cameraToWorld);
			}
		}
		
		for (laser l : lasers) {
			l.render(noisy_shader,cameraToWorld);
		}

		thief->render(shader, cameraToWorld);
    }

	// called every frame to move things
	void simulate() {
		if (gameStatus!=0) {
			return;
		}

		//Move character if user pressed a key
		move_character();

		//Collision detection
		bool collision = false;
		for (sprite &wall : sprites[2]) {
			collision = thief->collides_with(wall);
			if (collision) {
				break;
			}
		}
		thief->update(!collision);

		//Check if character is in one of the lasers and adjust laser accordingly / trigger alarm
		for (laser &l : lasers) {
			l.collides(*thief);
		}
	}

	//Character actions: 0=rotateLeft, 1=rotateRight, 2=forward, 3=backward
	void move_character() {
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
		layerPaths[1] += "\\museum_walls_shadows.csv";
		layerPaths[2] += "\\museum_walls.csv";
		layerPaths[3] += "\\museum_items.csv";
		path += "\\tilesheet.tga";

		//Read csv files with level definitions, and initialise sprites
		std::array<int, 1600> world;
		for (int i = 0; i < 3; i++) {
			readCsv(layerPaths[i], world);
			init_sprites(path, world, spriteSize, sprites[i]);
		}
		readCsv(layerPaths[3], world);
		init_sprites(path, world, spriteSize, sprites[3]);

		/*
		Load lasers. Different directions indicated by different type numbers in the world.
		108: laser up -> low
		123: laser low -> up
		124: laser right -> left
		107: laser left -> right
		*/
		int posX, posY;
		for (int id = 0; id < world.size(); ++id){
			int type = world[id];
				posX = (id%UNITS_X);
				posY = id / UNITS_Y;
				switch (type) {
				case 108:
					lasers.push_back(laser(vec2(posX, posY), vec2(0, -1), spriteSize, sprites));
					break;
				case 123:
					lasers.push_back(laser(vec2(posX, posY), vec2(0, 1), spriteSize, sprites));
					break;
				case 124:
					lasers.push_back(laser(vec2(posX, posY), vec2(-1, 0), spriteSize, sprites));
					break;
				case 107:
					lasers.push_back(laser(vec2(posX, posY), vec2(1, 0), spriteSize, sprites));
					break;
				}
		}

		// Load player character
		GLuint tex = resource_dict::get_texture_handle(GL_RGBA, "#ffffff");
		sprite thiefSprite;
		thiefSprite.init(tex, 1, 7, 0, spriteSize, spriteSize);
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

