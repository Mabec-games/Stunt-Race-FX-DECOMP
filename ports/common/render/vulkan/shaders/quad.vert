#version 450
// A picture's quad: target NDC and the picture's uv.
layout(location = 0) in vec4 a;
layout(location = 0) out vec2 v_uv;
void main() {
    gl_Position = vec4(a.x, -a.y, 0.0, 1.0);    // Vulkan's NDC has y down
    v_uv = a.zw;
}
