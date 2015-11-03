#pragma once
#include "sprite.h"
#include "fading_texture_shader.h"
namespace octet {

	//Class to represent a painting which could be either a side- or the main objective of the game. 
	//The class also acts as a container for all sprites associated with one painter, so they can be removed together when the painting is collected.
	class painting {
		std::vector<sprite> sprites;
		bool isMainObjective = false;

	public:
		painting(std::vector<sprite> sprites_, bool isMainObjective_) :sprites(sprites_), isMainObjective(isMainObjective_) {}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			for (sprite &s : sprites) {
				s.render(shader, cameraToWorld);
			}
		}

		void setIsMainObjective(bool isMainObjective_) {
			isMainObjective = isMainObjective_;
		}

		bool getIsMainObjective() {
			return isMainObjective;
		}

		std::vector<sprite>& getSprites() {
			return sprites;
		}

	};
}