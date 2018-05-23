#include "common/common2d.glsl"

#ifdef UMINEKO_SHADER_VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;
layout(location = 3) in float fadein;

out VertexData {
	vec2 texcoord;
	vec4 color;
	float fadein;
} vs_out;

void main() {
	vs_out.texcoord = texcoord;
	vs_out.color = color;
	vs_out.fadein = fadein;

	gl_Position = mat.projection * mat.view * mat.model * vec4(position.xy, 0, 1);
}

#endif

#ifdef UMINEKO_SHADER_FRAGMENT

in VertexData {
	vec2 texcoord;
	vec4 color;
	float fadein;
} fs_in;

out vec4 color;

layout(binding = 3) uniform Text {
	uniform vec4 progress;
} text;

uniform sampler2DRect tex;

void main() {
	color = fs_in.color * texture(tex, fs_in.texcoord).rrrr;
	color *= smoothstep(0.0, 1.0 - text.progress.x, 1.0 - fs_in.fadein);
}

#endif