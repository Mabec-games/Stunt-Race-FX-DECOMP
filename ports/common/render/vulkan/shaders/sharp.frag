#version 450
// Sharp bilinear: whole texels at any scale, blended only across the one
// target pixel a texel edge falls in (render/rr_size.h).
layout(set = 0, binding = 0) uniform sampler2D t;
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o;
void main() {
    vec2 ts = vec2(textureSize(t, 0)), px = v_uv * ts - 0.5;
    vec2 i = floor(px), f = px - i, d = max(fwidth(px), vec2(1e-5));
    f = clamp((f - 0.5) / d + 0.5, 0.0, 1.0);
    o = vec4(texture(t, (i + 0.5 + f) / ts).rgb, 1.0);
}
