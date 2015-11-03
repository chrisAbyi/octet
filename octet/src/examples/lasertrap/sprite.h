#pragma once

namespace octet {

	// Slightly modified sprite class from the Invaderers example
	// Modifications include getters and setters as well as a modified version of the collision function
	class sprite {

		// half the width of the sprite
		float halfWidth;

		// half the height of the sprite
		float halfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;

		int numRows, numCols, imgW, imgH, tileW, tileH;
		float uvs[8];

		// Store here so there's no need to decompose model-to-world matrix in order to infer rotation
		int rotation = 0;

	public:
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		sprite() {
			texture = 0;
			enabled = true;
		}

		void init(int _texture, float x, float y, float rotation, float w, float h) {
			x = (x * w) - (1 - w / 2);
			y = (y * h) - (1 - h / 2);
			y *= -1;

			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.rotate(rotation, 0, 0, 1);
			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
			uvs[0] = 0;
			uvs[1] = 0;
			uvs[2] = 1;
			uvs[3] = 0;
			uvs[4] = 1;
			uvs[5] = 1;
			uvs[6] = 0;
			uvs[7] = 1;
		}

		void init(int _texture, float x, float y, float rotation, float w, float h, int tileIdx, int tW, int tH, int iW, int iH) {

			x = (x * w) - (1 - w / 2);
			y = (y * h) - (1 - h / 2);
			y *= -1;

			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.rotate(rotation, 0, 0, 1);

			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;

			imgH = iH;
			imgW = iW;
			tileH = tH;
			tileW = tW;

			numRows = imgH / tileH;
			numCols = imgW / tileW;

			int xc = tileIdx % numCols;
			int	yc = tileIdx / numCols;

			float nw = (float)tileW / (float)imgW;
			float nh = (float)tileH / (float)imgH;

			float xn = (float)xc / (float)numCols;
			float yn = (float)yc / (float)numRows;

			//Upper left
			uvs[6] = xn;
			uvs[7] = 1.0f - yn;

			//Upper right
			uvs[4] = xn + nw;
			uvs[5] = 1.0f - yn;

			//Lower right
			uvs[2] = xn + nw;
			uvs[3] = 1.0f - (yn + nh);

			//Lower left
			uvs[0] = xn;
			uvs[1] = 1.0f - (yn + nh);
		}

		vec2 get_pos() const {
			return modelToWorld.row(3).xy();
		}

		int get_rot() {
			return rotation;
		}

		void set_pos(vec2 p) {
			modelToWorld.translate(vec3(p.x(), p.y(), 0.0f) - modelToWorld.row(3).xyz());
		}

		void set_rot(int _rotation) {
			modelToWorld.rotate(_rotation,0,0,1);
			rotation += _rotation;
			rotation %= 360;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

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

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-halfWidth, -halfHeight, 0,
				halfWidth, -halfHeight, 0,
				halfWidth, halfHeight, 0,
				-halfWidth, halfHeight, 0,
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			// attribute_uv is position in the texture of each corner
			// each corner (vertex) has 2 floats (x, y)
			// there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
			glEnableVertexAttribArray(attribute_uv);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// move the object
		void translate(float x, float y) {
			modelToWorld.translate(x, y, 0);
		}

		// move the object
		void translate(vec2 pos) {
			modelToWorld.translate(pos.x(), pos.y(), 0);
		}

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		// Modified version of collision function
		// Uses euclidean distance and allows to set minimum distance for collision to be true
		bool collides_with(const sprite &rhs, float factor) const {
			vec2 s1 = rhs.modelToWorld.row(3).xy();
			vec2 s0 = modelToWorld.row(3).xy();
			if (sqrt(pow(fabs(s0[0] - s1[0]), 2) + pow(fabs(s0[1] - s1[1]), 2)) < (halfWidth * factor)) {
				return true;
			}
			return false;
		}

		bool is_above(const sprite &rhs, float margin) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

			return
				(fabsf(dx) < halfWidth + margin)
				;
		}

		bool &is_enabled() {
			return enabled;
		}

	};
}