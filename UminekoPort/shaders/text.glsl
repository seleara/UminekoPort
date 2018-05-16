#include "common/common2d.glsl"

#ifdef UMINEKO_SHADER_VERTEX

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

out VertexData {
	vec2 texcoord;
	vec4 color;
} vs_out;

void main() {
	vs_out.texcoord = texcoord;
	vs_out.color = color;

	gl_Position = mat.projection * mat.view * mat.model * vec4(position.xy, 0, 1);
}

#endif

#ifdef UMINEKO_SHADER_FRAGMENT

in VertexData {
	vec2 texcoord;
	vec4 color;
} fs_in;

out vec4 color;

uniform sampler2DRect tex;

void main() {
	color = fs_in.color * texture(tex, fs_in.texcoord).rrrr;
}

#endif