#pragma once

#include "painting.h"
namespace octet {

	// Class representing the game's character
	// The separation of the character's sprite allows to check potential collisions of the character before the sprite gets updated
	class character {
		int rotation;
		sprite characterSprite;
		vec2 prevTrans, targetTrans;
		bool moved=false;

	public:

		void init(sprite _characterSprite) {
			rotation = 0;
			characterSprite = _characterSprite;
			prevTrans = vec2();
		}

		vec2 get_pos() const {
			return characterSprite.get_pos();
		}

		float get_rot() {
			return characterSprite.get_rot();
		}

		bool collides_with(sprite &s) {
			return characterSprite.collides_with(s, 1.5);
		}

		// Check whether character is close to a painting, by checking whether the character is close to any of the sprites associated with the painting
		bool close_to(painting &p) {
			std::vector<sprite> &sprites = p.getSprites();
			for (sprite &s : sprites) {
				// Abuse the collision function by means of setting a larger collision threshold. So this isn't really about collision, but proximity.
				if (characterSprite.collides_with(s, 2.5)) {
					return true;
				}
			}
			return false;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			characterSprite.render(shader, cameraToWorld);
		}

		// Rotating the character clockwise (0), counterclockwise (1) and moving it forward (2) and backward (3) 
		void move(int action) {

			//Only allow to move forward / backward once before an update of the associated sprite position
			if (moved) {
				return;
			}

			switch (action) {
			case 0:
				characterSprite.set_rot(10);
				break;
			case 1:
				characterSprite.set_rot(-10);
				break;
			case 2:
			{
				targetTrans = vec2(0.01f, 0.01f);
				moved = true;
				break;
			}
			case 3:
			{
				targetTrans = vec2(-0.01f, -0.01f);
				moved = true;
				break;
			}
			}
		}

		// Moves the sprite associated with the character forwards/backwards according to the movement operations triggered earlier.
		// Character might not be allowed to move. In this case, the planned movements are cancelled.
		void update(bool permitted)
		{
			if(moved && permitted){
				characterSprite.translate(targetTrans);
				prevTrans = targetTrans;
				moved = false;
			}
			if (!permitted) {
				//Move back a little bit
				targetTrans *= -1;
				characterSprite.translate(targetTrans);
				prevTrans = targetTrans;
				moved = false;
			}
		}

	};
}