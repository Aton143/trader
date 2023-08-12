#version 330 core
layout (location = 0) in vec3 input_vertex;
layout (location = 1) in vec4 input_color;
layout (location = 2) in vec2 input_uv;

uniform float uniform_scale;
uniform mat4  uniform_transform;

out vec4 vs_color;
out vec2 vs_uv;

void main()
{
  gl_Position = uniform_transform * vec4(input_vertex, 1.0);
  vs_color    = input_color * uniform_scale;
  vs_uv       = input_uv;
}
