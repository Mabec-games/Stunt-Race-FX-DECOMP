#version 450
// One to one.
layout(set = 0, binding = 0) uniform sampler2D t;
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o;
void main() {
    o = vec4(texelFetch(t, ivec2(v_uv * vec2(textureSize(t, 0))), 0).rgb, 1.0);
}
