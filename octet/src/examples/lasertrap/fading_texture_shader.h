#pragma once
namespace octet {

	//Extended texture_shader to overwrite fragment shader
	//In this version, the shader fades alpha smoothly from 0.5 to 1 and back
	class fading_texture_shader : public texture_shader {
		//To store the current execution time as seed
		GLuint phaseIndex_;
		float phase = 0;

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

			// modified the fragment shader: now uses a fade progress value phase supplied by the shader program's render method
			const char fragment_shader[] = SHADER_STR(
				varying vec2 uv_;
				uniform sampler2D sampler;
				uniform float phase;
			void main() {
				//Sine function used for smooth fading
				//Should generate alternating alpha values between 0.5..1
				float a = ((sin(phase)+1.0)/2.0);
				gl_FragColor = texture2D(sampler, uv_) * vec4(1, 1, 1, a);
			}
			);

			// use the common shader code to compile and link the shaders
			// the result is a shader program
			shader::init(vertex_shader, fragment_shader);

			// extract the indices of the uniforms to use later
			modelToProjectionIndex_ = glGetUniformLocation(program(), "modelToProjection");
			samplerIndex_ = glGetUniformLocation(program(), "sampler");
			phaseIndex_ = glGetUniformLocation(program(), "phase");
		}

		virtual void render(const mat4t &modelToProjection, int sampler) override{
			// tell openGL to use the program
			phase += 0.1;
			phase = fmod(phase, 6.283);
			shader::render();

			// customize the program with uniforms
			glUniform1i(samplerIndex_, sampler);
			glUniformMatrix4fv(modelToProjectionIndex_, 1, GL_FALSE, modelToProjection.get());

			// supply phase to shader as fading progress indicator
			glUniform1f(phaseIndex_, phase);
		}
	};
}