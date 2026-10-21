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
#include "compose_src.h"
#include "fill_src.h"

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

/* the compose pass: one triangle over the whole picture */
static const char *k_compose_vs =
    "#version 330 core\n"
    "void main() { vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2); gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0); }\n";

static GLuint s_copy, s_sharp, s_vao, s_vbo, s_fbo, s_col, s_pic;
/* the compositor: its program, the record's textures (units 0-6: the
   tiles, palettes, sprite tables, line states, the bits of the tiles'
   bytes from the 3D canvas, the kept copies of those bytes, and the kept
   copies of the characters' atlas), the composed picture and its
   framebuffer */
static GLuint s_compose, s_crec[7], s_cpic, s_cfbo;
/* the copy of the tiles' bytes rr_pack writes, its framebuffer, the kept
   copies' framebuffer; the pack's program and its runs */
static GLuint s_mirror, s_mirror_fbo, s_kept_fbo, s_pack, s_run_vao, s_run_vbo;
/* the characters' atlas, its framebuffer, the kept copies' framebuffer;
   the characters' program and their instances; the canvases' scale */
static GLuint s_atlas, s_atlas_fbo, s_akept_fbo, s_chars, s_char_vao, s_char_vbo;
static int s_scale = 1;
/* the 3D canvas: its program, the ROM (unit 0), the canvas and its
   framebuffer, the spans' vertex array and buffer */
static GLuint s_fill, s_rom, s_canvas[RR_CANVASES], s_canvas_fbo[RR_CANVASES], s_span_vao, s_span_vbo;
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

static GLuint program(const char *vs, const char *fs)
{
    GLuint p = glCreateProgram(), v = shader(GL_VERTEX_SHADER, vs), f = shader(GL_FRAGMENT_SHADER, fs);
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
    int w = 0, h = 0, i;
    if (!gl_ctx_open(win) || !gl_api_load())
        return 0;
    s_window = win != NULL;
    s_copy = program(k_vs, k_copy);
    s_sharp = program(k_vs, k_sharp);
    s_compose = program(k_compose_vs, k_compose_gl);
    if (!s_copy || !s_sharp || !s_compose)
        return 0;
    {
        static const char *const names[7] = { "t_vram", "t_pal", "t_oam", "t_line", "t_ext", "t_mirror", "t_atlas" };
        static const GLint fmt[7] = { GL_R16UI, GL_R16UI, GL_R8UI, GL_R32UI, GL_R32UI, GL_R8UI, GL_R16UI };
        static const GLenum type[7] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT,
                                        GL_UNSIGNED_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT };
        const int tw[7] = { 256, 256, 544, RR_LINE_WORDS, 256, 256, 512 * s_scale };
        const int th[7] = { 128 * RR_VERSIONS, 225, RR_VERSIONS, 225, 8 * RR_VERSIONS, 256 * RR_VERSIONS,
                            256 * s_scale * RR_VERSIONS };
        static const GLuint zero[4] = { 0, 0, 0, 0 };
        glGenTextures(7, s_crec);
        for (i = 0; i < 7; i++) {
            glUniform1i(glGetUniformLocation(s_compose, names[i]), i);
            glBindTexture(GL_TEXTURE_2D, s_crec[i]);
            /* integer textures: complete only without mipmaps or filtering */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, fmt[i], tw[i], th[i], 0, GL_RED_INTEGER, type[i], NULL);
        }
        glGenTextures(1, &s_cpic);
        glBindTexture(GL_TEXTURE_2D, s_cpic);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 224, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &s_cfbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_cfbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_cpic, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "rr: gl: the compositor's target is not complete\n");
            return 0;
        }
        glGenTextures(1, &s_mirror);
        glBindTexture(GL_TEXTURE_2D, s_mirror);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, 256, 256, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, &s_mirror_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_mirror_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_mirror, 0);
        glGenFramebuffers(1, &s_kept_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_kept_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_crec[5], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "rr: gl: the tiles' copies are not complete\n");
            return 0;
        }
        glGenTextures(1, &s_atlas);
        glBindTexture(GL_TEXTURE_2D, s_atlas);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, 512 * s_scale, 256 * s_scale, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
        glGenFramebuffers(1, &s_atlas_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_atlas_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_atlas, 0);
        glClearBufferuiv(GL_COLOR, 0, zero);    /* every slot invalid until a character is made in it */
        glGenFramebuffers(1, &s_akept_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_akept_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_crec[6], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "rr: gl: the atlas's copies are not complete\n");
            return 0;
        }
    }
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

/* texture tex (w x h, on unit 0) drawn over target rectangle d: one to one
   when d is w x h, else sharp bilinear */
static void draw(GLuint tex, int w, int h, const int d[4])
{
    static const int ix[6] = { 0, 1, 2, 2, 1, 3 };
    float c[4][4] = { { (float)d[0], (float)d[1], 0, 0 }, { (float)d[2], (float)d[1], 1, 0 },
                      { (float)d[0], (float)d[3], 0, 1 }, { (float)d[2], (float)d[3], 1, 1 } };
    float q[6][4];
    int i;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
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

void rr_image(const uint32_t *argb, int w, int h, const int d[4])
{
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
    draw(s_pic, w, h, d);
}

/* the picture composed into s_cpic (its row 0 the first line, as the
   pictures are uploaded), then drawn over d as rr_image draws */
void rr_compose(const RrPicture *p, const int d[4])
{
    int i, v = p->versions < 1 ? 1 : p->versions > RR_VERSIONS ? RR_VERSIONS : p->versions;
    const void *src[5] = { p->vram, p->palette, p->oam, p->lines, p->external };
    static const GLenum type[5] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT, GL_UNSIGNED_INT };
    const int tw[5] = { 256, 256, 544, RR_LINE_WORDS, 256 }, th[5] = { 128 * v, 225, v, 225, 8 * v };
    if (d[2] <= d[0] || d[3] <= d[1])
        return;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (i = 0; i < 5; i++) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)i);
        glBindTexture(GL_TEXTURE_2D, s_crec[i]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw[i], th[i], GL_RED_INTEGER, type[i], src[i]);
    }
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, s_crec[5]);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, s_crec[6]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, s_cfbo);
    glViewport(0, 0, 256, 224);
    glUseProgram(s_compose);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_rw, s_rh);
    draw(s_cpic, 256, 224, d);
}

/* the 3D canvas: the spans as instances (two integer vec4s each), the
   canvas row 0 the framebuffer's first row */
int rr_fill_rom(const uint8_t *rom, uint32_t size)
{
    if (size < 1024 || size % 1024)
        return 0;
    if (!s_fill) {
        GLuint u;
        s_fill = program(k_fill_gl_vert, k_fill_gl_frag);
        if (!s_fill)
            return 0;
        glUseProgram(s_fill);
        glUniform1i(glGetUniformLocation(s_fill, "t_rom"), 0);
        glGenTextures(RR_CANVASES, s_canvas);
        glGenFramebuffers(RR_CANVASES, s_canvas_fbo);
        for (u = 0; u < RR_CANVASES; u++) {
            glBindTexture(GL_TEXTURE_2D, s_canvas[u]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, 256, 256, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
            glBindFramebuffer(GL_FRAMEBUFFER, s_canvas_fbo[u]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_canvas[u], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                fprintf(stderr, "rr: gl: the 3D canvas is not complete\n");
                return 0;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        s_pack = program(k_pack_gl_vert, k_pack_gl_frag);
        if (!s_pack)
            return 0;
        glUseProgram(s_pack);
        glUniform1i(glGetUniformLocation(s_pack, "t_canvas"), 0);
        glGenVertexArrays(1, &s_run_vao);
        glBindVertexArray(s_run_vao);
        glGenBuffers(1, &s_run_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_run_vbo);
        glVertexAttribIPointer(0, 4, GL_UNSIGNED_INT, RR_RUN_WORDS * 4, (const void *)0);
        glVertexAttribDivisor(0, 1);
        glEnableVertexAttribArray(0);
        s_chars = program(k_char_gl_vert, k_char_gl_frag);
        if (!s_chars)
            return 0;
        glUseProgram(s_chars);
        glUniform1i(glGetUniformLocation(s_chars, "t_canvas"), 0);
        glGenVertexArrays(1, &s_char_vao);
        glBindVertexArray(s_char_vao);
        glGenBuffers(1, &s_char_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_char_vbo);
        glVertexAttribIPointer(0, 4, GL_UNSIGNED_INT, RR_CHAR_WORDS * 4, (const void *)0);
        glVertexAttribDivisor(0, 1);
        glEnableVertexAttribArray(0);
        glGenVertexArrays(1, &s_span_vao);
        glBindVertexArray(s_span_vao);
        glGenBuffers(1, &s_span_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_span_vbo);
        for (u = 0; u < 2; u++) {
            glVertexAttribIPointer(u, 4, GL_UNSIGNED_INT, RR_SPAN_WORDS * 4, (const void *)(size_t)(u * 16));
            glVertexAttribDivisor(u, 1);
            glEnableVertexAttribArray(u);
        }
        glBindVertexArray(s_vao);
        glGenTextures(1, &s_rom);
    }
    glBindTexture(GL_TEXTURE_2D, s_rom);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, 1024, (GLsizei)(size / 1024), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, rom);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    return 1;
}

void rr_fill(const uint32_t *spans, int n, int clear, int canvas)
{
    static const GLuint zero[4] = { 0, 0, 0, 0 };
    size_t bytes = (size_t)n * RR_SPAN_WORDS * 4;
    if (!s_fill)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, s_canvas_fbo[canvas]);
    glViewport(0, 0, 256, 256);
    if (clear)
        glClearBufferuiv(GL_COLOR, 0, zero);
    if (n > 0) {
        glBindVertexArray(s_span_vao);
        glBindBuffer(GL_ARRAY_BUFFER, s_span_vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, spans, GL_STREAM_DRAW);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_rom);
        glUseProgram(s_fill);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 12, n);
        glBindVertexArray(s_vao);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_rw, s_rh);
}

/* the runs as instances (an integer vec4 each), into the copy's rows */
void rr_pack(const uint32_t *runs, int n, int canvas)
{
    if (!s_pack || n <= 0)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, s_mirror_fbo);
    glViewport(0, 0, 256, 256);
    glBindVertexArray(s_run_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_run_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)n * RR_RUN_WORDS * 4), runs, GL_STREAM_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_canvas[canvas]);
    glUseProgram(s_pack);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, n);
    glBindVertexArray(s_vao);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_rw, s_rh);
}

void rr_chars(const uint32_t *chars, int n, int canvas)
{
    static uint32_t *w;
    static int cap;
    int i;
    if (!s_chars || n <= 0)
        return;
    if (n > cap) {
        cap = n * 2;
        w = (uint32_t *)realloc(w, (size_t)cap * RR_CHAR_WORDS * 4);
    }
    memcpy(w, chars, (size_t)n * RR_CHAR_WORDS * 4);
    for (i = 0; i < n; i++) {                   /* the scale and the atlas's size, for the shader */
        w[i * RR_CHAR_WORDS + 2] = (uint32_t)s_scale;
        w[i * RR_CHAR_WORDS + 3] = (uint32_t)(512 * s_scale) | (uint32_t)(256 * s_scale) << 16;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_atlas_fbo);
    glViewport(0, 0, 512 * s_scale, 256 * s_scale);
    glBindVertexArray(s_char_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_char_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)n * RR_CHAR_WORDS * 4), w, GL_STREAM_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_canvas[canvas]);
    glUseProgram(s_chars);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, n);
    glBindVertexArray(s_vao);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, s_rw, s_rh);
}

void rr_pack_keep(int version)
{
    if (!s_pack || version < 0 || version >= RR_VERSIONS)
        return;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_mirror_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_kept_fbo);
    glBlitFramebuffer(0, 0, 256, 256, 0, 256 * version, 256, 256 * version + 256, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_atlas_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_akept_fbo);
    glBlitFramebuffer(0, 0, 512 * s_scale, 256 * s_scale, 0, 256 * s_scale * version, 512 * s_scale,
                      256 * s_scale * (version + 1), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
}

int rr_pack_read(uint8_t *bytes)
{
    if (!s_pack)
        return 0;
    glBindFramebuffer(GL_FRAMEBUFFER, s_mirror_fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 256, 256, GL_RED_INTEGER, GL_UNSIGNED_BYTE, bytes);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    return 1;
}

int rr_fill_read(uint16_t *canvas)
{
    if (!s_fill)
        return 0;
    glBindFramebuffer(GL_FRAMEBUFFER, s_canvas_fbo[0]);
    glPixelStorei(GL_PACK_ALIGNMENT, 2);
    glReadPixels(0, 0, 256, 256, GL_RED_INTEGER, GL_UNSIGNED_SHORT, canvas);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    return 1;
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
