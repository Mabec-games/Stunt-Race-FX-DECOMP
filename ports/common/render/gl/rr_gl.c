/*
 * rr_gl.c: the renderer backend on OpenGL 3.3 core (Windows, on the host's
 * window; macOS, offscreen only, for checking it there: the Mac's windows
 * draw with Metal), the Boss Rally port's design: the frame is drawn into a
 * framebuffer object the size of the window, then copied onto the window's
 * back buffer and swapped. Pictures go on with the sharp bilinear shader
 * (rr_size.h's weight; fwidth gives the texels to one target pixel).
 *
 * GL keeps rows bottom up: the frame is drawn with its top at NDC +1, so the
 * framebuffer object's last row is the picture's first, and a read back
 * turns the rows over.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl_api.h"
#include "rr.h"
#include "rr_size.h"
#include "host.h"

/* the host's window for GL (the Windows host's HWND), its size in pixels;
   without one, offscreen */
void *host_window_handle(void);
void host_window_pixels(int *w, int *h);
#if !defined(_WIN32)          /* (the Windows hosts, windowed and null, give both) */
__attribute__((weak)) void *host_window_handle(void) { return NULL; }
__attribute__((weak)) void host_window_pixels(int *w, int *h) { *w = *h = 0; }
#endif

static const char *k_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec4 a;\n"
    "out vec2 uv;\n"
    "void main() { gl_Position = vec4(a.xy, 0.0, 1.0); uv = a.zw; }\n";
/* one to one */
static const char *k_copy =
    "#version 330 core\n"
    "uniform sampler2D t;\n"
    "in vec2 uv;\n"
    "out vec4 o;\n"
    "void main() { o = vec4(texelFetch(t, ivec2(uv * vec2(textureSize(t, 0))), 0).rgb, 1.0); }\n";
/* sharp bilinear: whole texels at any scale, blended only across the one
   target pixel a texel edge falls in */
static const char *k_sharp =
    "#version 330 core\n"
    "uniform sampler2D t;\n"
    "in vec2 uv;\n"
    "out vec4 o;\n"
    "void main() {\n"
    "  vec2 ts = vec2(textureSize(t, 0)), px = uv * ts - 0.5;\n"
    "  vec2 i = floor(px), f = px - i, d = max(fwidth(px), vec2(1e-5));\n"
    "  f = clamp((f - 0.5) / d + 0.5, 0.0, 1.0);\n"
    "  o = vec4(texture(t, (i + 0.5 + f) / ts).rgb, 1.0);\n"
    "}\n";

static GLuint s_copy, s_sharp, s_vao, s_vbo, s_fbo, s_col, s_pic;
static int s_pw, s_ph;
static int s_rw, s_rh, s_window;

static GLuint shader(GLenum kind, const char *src)
{
    GLuint s = glCreateShader(kind);
    GLint ok = 0;
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "rr: gl shader: %s\n", log);
        return 0;
    }
    return s;
}

static GLuint program(const char *fs)
{
    GLuint p = glCreateProgram(), v = shader(GL_VERTEX_SHADER, k_vs), f = shader(GL_FRAGMENT_SHADER, fs);
    GLint ok = 0;
    if (!v || !f)
        return 0;
    glAttachShader(p, v);
    glAttachShader(p, f);
    glBindAttribLocation(p, 0, "a");
    glBindFragDataLocation(p, 0, "o");
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof log, NULL, log);
        fprintf(stderr, "rr: gl program: %s\n", log);
        return 0;
    }
    glUseProgram(p);
    glUniform1i(glGetUniformLocation(p, "t"), 0);
    return p;
}

static void targets(int w, int h)
{
    if (!s_fbo)
        glGenFramebuffers(1, &s_fbo);
    if (!s_col)
        glGenTextures(1, &s_col);
    glBindTexture(GL_TEXTURE_2D, s_col);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_col, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr, "rr: gl: the target is not complete\n");
    s_rw = w;
    s_rh = h;
}

int rr_open(void)
{
    void *win = host_window_handle();
    int w = 0, h = 0;
    if (!gl_ctx_open(win) || !gl_api_load())
        return 0;
    s_window = win != NULL;
    s_copy = program(k_copy);
    s_sharp = program(k_sharp);
    if (!s_copy || !s_sharp)
        return 0;
    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);
    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 16, NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, (const void *)0);
    glEnableVertexAttribArray(0);
    glGenTextures(1, &s_pic);
    glBindTexture(GL_TEXTURE_2D, s_pic);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (s_window)
        host_window_pixels(&w, &h);
    if (w < 1 || h < 1)
        rr_headless_size(&w, &h);
    targets(w, h);
    return 1;
}

void rr_close(void) {}

const char *rr_name(void) { return "gl"; }

void rr_target(int *w, int *h)
{
    if (s_window) {
        int fw, fh;
        host_window_pixels(&fw, &fh);
        if (fw > 0 && fh > 0 && (fw != s_rw || fh != s_rh))
            targets(fw, fh);
    }
    *w = s_rw;
    *h = s_rh;
}

void rr_begin(uint32_t argb)
{
    int w, h;
    rr_target(&w, &h);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_rw, s_rh);
    glClearColor(((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f, (argb & 0xFF) / 255.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void rr_image(const uint32_t *argb, int w, int h, const int d[4])
{
    static const int ix[6] = { 0, 1, 2, 2, 1, 3 };
    float c[4][4] = { { (float)d[0], (float)d[1], 0, 0 }, { (float)d[2], (float)d[1], 1, 0 },
                      { (float)d[0], (float)d[3], 0, 1 }, { (float)d[2], (float)d[3], 1, 1 } };
    float q[6][4];
    int i;
    if (w <= 0 || h <= 0 || d[2] <= d[0] || d[3] <= d[1])
        return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_pic);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (w != s_pw || h != s_ph) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, argb);
        s_pw = w;
        s_ph = h;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, argb);
    }
    for (i = 0; i < 6; i++) {
        q[i][0] = c[ix[i]][0] / s_rw * 2 - 1;
        q[i][1] = 1 - c[ix[i]][1] / s_rh * 2;
        q[i][2] = c[ix[i]][2];
        q[i][3] = c[ix[i]][3];
    }
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof q, q, GL_STREAM_DRAW);
    glUseProgram(d[2] - d[0] == w && d[3] - d[1] == h ? s_copy : s_sharp);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

/* The compositor (render/compose/core.glsl): not yet on this backend; it
   draws nothing (the shader source is generated for it: compose_src.h's
   k_compose_gl, vulkan/compose_spv.h). */
void rr_compose(const RrPicture *p, const int d[4])
{
    (void)p;
    (void)d;
}

void rr_present(void)
{
    if (s_window) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, s_rw, s_rh, 0, 0, s_rw, s_rh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        gl_ctx_swap();
    }
}

int rr_read(uint32_t *argb)
{
    int y;
    uint32_t *row = (uint32_t *)malloc((size_t)s_rw * 4);
    if (!row)
        return 0;
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, s_rw, s_rh, GL_BGRA, GL_UNSIGNED_BYTE, argb);
    for (y = 0; y < s_rh / 2; y++) {           /* rows top first */
        uint32_t *a = argb + (size_t)y * (size_t)s_rw, *b = argb + (size_t)(s_rh - 1 - y) * (size_t)s_rw;
        memcpy(row, a, (size_t)s_rw * 4);
        memcpy(a, b, (size_t)s_rw * 4);
        memcpy(b, row, (size_t)s_rw * 4);
    }
    free(row);
    return 1;
}
