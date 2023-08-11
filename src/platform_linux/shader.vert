#version 330 core
layout (location = 0) in vec3 input_vertex;
layout (location = 1) in vec4 input_color;

out vec4 vs_color;

void main()
{
  gl_Position = vec4(input_vertex, 1.0);
  vs_color    = input_color;
}
