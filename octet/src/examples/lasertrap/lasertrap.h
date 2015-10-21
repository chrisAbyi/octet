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
	
	class noisy_texture_shader : public texture_shader {
		public:
		void init(){
			// kept the vertex shader     
			const char vertex_shader[] = SHADER_STR(
				varying vec2 uv_;

			attribute vec4 pos;
			attribute vec2 uv;

			uniform mat4 modelToProjection;

			void main() { gl_Position = modelToProjection * pos; uv_ = uv; }
			);

			// modified the fragment shader
			const char fragment_shader[] = SHADER_STR(
				varying vec2 uv_;
				uniform sampler2D sampler;
				void main() { gl_FragColor = texture2D(sampler, uv_); }
			);

			// use the common shader code to compile and link the shaders
			// the result is a shader program
			shader::init(vertex_shader, fragment_shader);

			// extract the indices of the uniforms to use later
			modelToProjectionIndex_ = glGetUniformLocation(program(), "modelToProjection");
			samplerIndex_ = glGetUniformLocation(program(), "sampler");
		}
	};

	class laser {
		vec2 p0, p1, p2, p3;
		mat4t modelToWorld; //where is the laser beam?
		const std::array<int, 1600> &world;
		float unitSize;
		GLuint texture;

		void calcCoordinates(const vec2 &positionOrigin, const vec2 &direction) {

			//First check where laser hits a wall with respect to its direction
			vec2 positionHit = positionOrigin;
			int id = positionHit[1] * UNITS_X + positionHit[0];

			while (world[id] != 0 && world[id] != 1) {
				positionHit += direction;
				id = positionHit[1] * UNITS_X + positionHit[0];
			}

			//Transform abstract coordinates to model-view coordinates
			float t = (1 - unitSize / 2);
			p0 = ((positionOrigin-direction/2) * unitSize) - t;
			p0[1] *= -1;

			p1 = ((positionHit-direction/2) * unitSize) - t;
			p1[1] *= -1;

			//Add width to the laser, i.e. 2 more points in space
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

	public:
		laser(const vec2 _positionOrigin, const vec2 _direction, const float _unitSize, const std::array<int, 1600> &_world) : unitSize(_unitSize), world(_world) {
			calcCoordinates(_positionOrigin, _direction);
			texture = resource_dict::get_texture_handle(GL_RGBA, "#ff0000");
		}

		void render(mat4t &cameraToWorld) {
			noisy_texture_shader shader;
			shader.init();
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
	std::array<int, 1600> world;
	std::vector<laser> lasers;
	std::shared_ptr<character> thief;
	sprite thiefSprite;

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
			l.render(cameraToWorld);
		}

		thief->render(shader, cameraToWorld);
		
    }

	// called every frame to move things
	void simulate() {
		if (gameStatus!=0) {
			return;
		}

		//Collision detection
		/*
		bool collision = false;
		for (sprite wall : sprites[1]) {
			collision = thiefSprite.collides_with(wall);
			if (collision) {
				std::cout << "collided!" << std::endl;
				break;
			}
		}*/
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

		// Load player character
		GLuint tex = resource_dict::get_texture_handle(GL_RGBA, "#ffffff");
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

