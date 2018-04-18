// This GLSL file contains common definitions and functions for 2D shaders

#ifdef UMINEKO_COMMON_3D
#error Due to some name clashes, both 'common.glsl' and 'common2d.glsl' are not allowed to be included in the same shader.
#endif

#define UMINEKO_COMMON_2D

layout(binding = 1) uniform Matrices2D {
	uniform mat4 projection;
	uniform mat4 view;
	uniform mat4 model;
} mat;