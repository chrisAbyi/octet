namespace octet {

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

		bool collides_with(sprite &s) {
			return characterSprite.collides_with(s);
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			characterSprite.render(shader, cameraToWorld);
		}

		void move(int action) {

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
				float rad = rotation*(3.14159265f / 180);
				float x = cosf(rad)*0.01f;
				float y = sinf(rad)*0.01f;
				targetTrans = vec2(x, y);
				moved = true;
				break;
			}
			case 3:
			{
				float rad = rotation*(3.14159265f / 180);
				float x = cosf(rad)*0.01f;
				float y = sinf(rad)*0.01f;
				targetTrans = vec2(-x, -y);
				moved = true;
				break;
			}
			}
		}

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