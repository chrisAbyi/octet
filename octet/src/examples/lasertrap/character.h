namespace octet {

	class character {
		int rotation;
		const std::array<int, 1600> &world;
		sprite characterSprite;

	public:
		character(const std::array<int, 1600> &_world) : world(_world) {}

		/*
		void init(vec2 initialPosition, int initialRotation, sprite _characterSprite)
		{
		position = initialPosition;
		rotation = initialRotation;
		characterSprite = _characterSprite;
		}*/

		void init(sprite _characterSprite) {
			rotation = 0;
			characterSprite = _characterSprite;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			characterSprite.render(shader, cameraToWorld);
		}

		void rotateLeft() {
			characterSprite.set_rot(5);
		}

		void rotateRight() {
			characterSprite.set_rot(-5);
		}

		void moveForward() {
			float rad = rotation*(3.14159265f / 180);
			float x = cosf(rad)*0.01f;
			float y = sinf(rad)*0.01f;
			characterSprite.translate(vec2(x, y));
		}

		void moveBackward() {
			float rad = rotation*(3.14159265f / 180);
			float x = cosf(rad)*0.01f;
			float y = sinf(rad)*0.01f;
			characterSprite.translate(vec2(x, y));
		}

	};
}