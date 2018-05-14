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

layout(binding = 2) uniform Transition {
	uniform vec4 progress;
} trans;

layout(binding = 0) uniform sampler2D prevTex;
layout(binding = 1) uniform sampler2D nextTex;
layout(binding = 2) uniform sampler2D maskTex;

void main() {
	vec4 prev = texture(prevTex, fs_in.texcoord);
	vec4 next = texture(nextTex, fs_in.texcoord);
	if (trans.progress.y) {
		vec4 mask = texture(maskTex, fs_in.texcoord);
		vec4 val = mix(prev, next, smoothstep(mask.r, 1.0, trans.progress.x));
		color = fs_in.color * val;
	} else {
		color = fs_in.color * mix(prev, next, trans.progress.x);
	}
}

#endif