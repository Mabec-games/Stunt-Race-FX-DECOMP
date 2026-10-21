layout(location = 0) in uvec4 a_run;
flat out uint r0, r1, r2;
void main()
{
    uint p = run_corner(a_run.x, uint(gl_VertexID));
    // the copy's row 0 at NDC -1: the framebuffer's first row in memory
    gl_Position = vec4(float(p & 0xFFFFu) / 128.0 - 1.0, float(p >> 16) / 128.0 - 1.0, 0.0, 1.0);
    r0 = a_run.x; r1 = a_run.y; r2 = a_run.z;
}
