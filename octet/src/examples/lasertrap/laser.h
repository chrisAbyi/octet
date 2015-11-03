#pragma once
#include "noisy_texture_shader.h"
namespace octet {

	// Class representing a laser in the game
	// Comprises methods to readjust the laser according to objects turning up between its origin and the closest wall
	class laser {
		vec2 p0, p1, p2, p3;
		vec2 origin, hit, wallHit;
		mat4t modelToWorld;
		const std::vector<sprite> &walls;
		const std::vector<sprite> &mirrors;
		float unitSize;
		GLuint texture;
		const vec2 direction;

		// Check where laser hits the closest wall with respect to its orientation
		void calcWallHit() {

			wallHit = origin;
			boolean collided = false;
			do {
				wallHit += direction*unitSize;
				for (sprite s : walls) {

					if (isSame(s.get_pos(), wallHit)) {
						collided = true;
						break;
					}
				}
			} while (!collided);

		}

		// Calculate the dimensions of the rectangle that represents the laser
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

		// Checks if two vec2 are the same by means of epsilon-float-comparison
		inline bool isSame(const vec2 v0, const vec2 v1) const {
			if (fabs(v0[0] - v1[0]) < 0.001) {
				if (fabs(v0[1] - v1[1]) < 0.001) {
					return true;
				}
			}
			return false;
		}

	public:

		// Sets up laser. Requires position where the laser originates and its direction as a vec2 object, e.g. (0,1) if positive on y axis only.
		// As part of the setup, it will be calculated where the laser hits the next wall
		laser(const GLuint texture_, const vec2 _positionOrigin, const vec2 _direction, const float _unitSize, const std::vector<sprite> &_walls, const std::vector<sprite> &_mirrors) : texture(texture_), unitSize(_unitSize), walls(_walls), mirrors(_mirrors), direction(_direction) {

			//Transform abstract coordinates to model-view coordinates
			origin = (_positionOrigin * unitSize) - (1 - unitSize / 2);
			origin[1] *= -1;

			//Set-up wall coordinates and assign them to laser
			calcWallHit();
			hit = wallHit;
			calcRectangle();
		}

		// Check where laser hits an object at a certain position with respect to its orientation
		// If the collision is positive, the laser's length will be adjusted and the laser sprite's dimensions will be recalculated
		bool collides(const vec2 cPos) {

			//Outer conditional checks whether sprite close to laser x/y-axis. Inner conditional checks whether sprite between origin and hit point of laser.
			if (fabs(cPos[0] - origin[0]) < unitSize / 2 && fabs(cPos[0] - hit[0]) < unitSize / 2) {
				if ((fabs(cPos[1] - origin[1]) < fabs(origin[1] - hit[1])) && (fabs(cPos[1] - hit[1]) < fabs(origin[1] - hit[1]))) {
					hit = vec2(hit[0], cPos[1]);
					calcRectangle();
					return true;
				}
			}
			else if (fabs(cPos[1] - origin[1]) < unitSize / 2 && fabs(cPos[1] - hit[1]) < unitSize / 2) {
				if ((fabs(cPos[0] - origin[0]) < fabs(origin[0] - hit[0])) && (fabs(cPos[0] - hit[0]) < fabs(origin[0] - hit[0]))) {
					hit = vec2(cPos[0], hit[1]);
					calcRectangle();
					return true;
				}
			}
			return false;
		}

		// Resets the position of last hit to the position of the wall
		// To be called when an object that remained in the laser is removed or pulled away
		void reset() {
 			hit = wallHit;
			calcRectangle();
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
}