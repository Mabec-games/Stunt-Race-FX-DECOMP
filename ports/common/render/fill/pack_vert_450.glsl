layout(location = 0) in uvec4 a_run;
layout(location = 0) flat out uint r0;
layout(location = 1) flat out uint r1;
layout(location = 2) flat out uint r2;
void main()
{
    uint p = run_corner(a_run.x, uint(gl_VertexIndex));
    // Vulkan's y runs down: the copy's row 0 at NDC -1 is the top
    gl_Position = vec4(float(p & 0xFFFFu) / 128.0 - 1.0, float(p >> 16) / 128.0 - 1.0, 0.0, 1.0);
    r0 = a_run.x; r1 = a_run.y; r2 = a_run.z;
}
