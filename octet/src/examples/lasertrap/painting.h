#pragma once

#include "sprite.h"
namespace octet {

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