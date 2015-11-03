#pragma once
namespace octet {

	//Extended texture_shader to overwrite fragment shader
	//In this version, the shader flickers / shows noise
	class noisy_texture_shader : public texture_shader {
		GLuint randomIndex_;

	public:
		void init() {
			// kept the vertex shader     
			const char vertex_shader[] = SHADER_STR(
				varying vec2 uv_;

			attribute vec4 pos;
			attribute vec2 uv;
			uniform mat4 modelToProjection;

			void main() { gl_Position = modelToProjection * pos; uv_ = uv; }
			);

			// modified the fragment shader: now uses a random uniform supplied by the shader program's render method
			const char fragment_shader[] = SHADER_STR(
			varying vec2 uv_;
			uniform int random;
			uniform sampler2D sampler;

			//Modified fragment shader introducing noise: multiplies each pixel of the original texture by a grey transparent color, which slightly varies in brightness
			void main() {
				//Essentially a very chaotic hash function for the uv coordinates to represent noise. Introducing a random float on every frame makes this time-variant. Should generate pseudorandom numbers in [0.7,1], that are different for each pixel.
				float r = fract(sin(dot(uv_*float(random), vec2(12.9898, 78.233))) * 43758.5453) * 0.3 + 0.7;
				gl_FragColor = texture2D(sampler, uv_) * vec4(r, r, r, 0.7);
			}
			);

			// use the common shader code to compile and link the shaders
			// the result is a shader program
			shader::init(vertex_shader, fragment_shader);

			// extract the indices of the uniforms to use later
			modelToProjectionIndex_ = glGetUniformLocation(program(), "modelToProjection");
			samplerIndex_ = glGetUniformLocation(program(), "sampler");
			randomIndex_ = glGetUniformLocation(program(), "random");
		}

		virtual void render(const mat4t &modelToProjection, int sampler) override {
			// tell openGL to use the program
			shader::render();

			// customize the program with uniforms
			glUniform1i(samplerIndex_, sampler);
			glUniformMatrix4fv(modelToProjectionIndex_, 1, GL_FALSE, modelToProjection.get());

			// supply random number to shader
			glUniform1i(randomIndex_, rand());
		}
	};
}