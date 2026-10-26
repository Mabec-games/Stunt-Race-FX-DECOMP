/*
 * launcher.c: Stunt Race FX's launcher (SDL3): the player's ROM chosen and
 * checked, the port's options set, the game started with them and the
 * launcher shown again when it ends. Settings in
 * $XDG_CONFIG_HOME/stuntracefx/launcher.cfg (~/.config/...).
 *
 * Keyboard (arrows, Return, Escape), game controller (d-pad, A, B) or
 * mouse. The game is the `stuntracefx` beside this program; the
 * background `launcher-bg.bmp` beside it or in ../share/stuntracefx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <SDL3/SDL.h>

#define LW 512                  /* the launcher's own pixels (scaled to the window) */
#define LH 320

/* ---- the font: 7 x 7 pixel letters, drawn chunky with a shadow ---- */
typedef struct { char c; const char *rows[7]; } Glyph;
static const Glyph font[] = {
    {'A', {" ##### ", "##   ##", "##   ##", "#######", "##   ##", "##   ##", "##   ##"}},
    {'B', {"###### ", "##   ##", "##   ##", "###### ", "##   ##", "##   ##", "###### "}},
    {'C', {" ######", "##     ", "##     ", "##     ", "##     ", "##     ", " ######"}},
    {'D', {"###### ", "##   ##", "##   ##", "##   ##", "##   ##", "##   ##", "###### "}},
    {'E', {"#######", "##     ", "##     ", "###### ", "##     ", "##     ", "#######"}},
    {'F', {"#######", "##     ", "##     ", "###### ", "##     ", "##     ", "##     "}},
    {'G', {" ######", "##     ", "##     ", "##  ###", "##   ##", "##   ##", " ######"}},
    {'H', {"##   ##", "##   ##", "##   ##", "#######", "##   ##", "##   ##", "##   ##"}},
    {'I', {"#######", "  ###  ", "  ###  ", "  ###  ", "  ###  ", "  ###  ", "#######"}},
    {'J', {"    ###", "     ##", "     ##", "     ##", "##   ##", "##   ##", " ##### "}},
    {'K', {"##   ##", "##  ## ", "## ##  ", "####   ", "## ##  ", "##  ## ", "##   ##"}},
    {'L', {"##     ", "##     ", "##     ", "##     ", "##     ", "##     ", "#######"}},
    {'M', {"##   ##", "### ###", "#######", "## # ##", "##   ##", "##   ##", "##   ##"}},
    {'N', {"##   ##", "###  ##", "#### ##", "## ####", "##  ###", "##   ##", "##   ##"}},
    {'O', {" ##### ", "##   ##", "##   ##", "##   ##", "##   ##", "##   ##", " ##### "}},
    {'P', {"###### ", "##   ##", "##   ##", "###### ", "##     ", "##     ", "##     "}},
    {'Q', {" ##### ", "##   ##", "##   ##", "##   ##", "## # ##", "##  ## ", " #### #"}},
    {'R', {"###### ", "##   ##", "##   ##", "###### ", "## ##  ", "##  ## ", "##   ##"}},
    {'S', {" ######", "##     ", "##     ", " ##### ", "     ##", "     ##", "###### "}},
    {'T', {"#######", "  ###  ", "  ###  ", "  ###  ", "  ###  ", "  ###  ", "  ###  "}},
    {'U', {"##   ##", "##   ##", "##   ##", "##   ##", "##   ##", "##   ##", " ##### "}},
    {'V', {"##   ##", "##   ##", "##   ##", "##   ##", " ## ## ", " ## ## ", "  ###  "}},
    {'W', {"##   ##", "##   ##", "##   ##", "## # ##", "#######", "### ###", "##   ##"}},
    {'X', {"##   ##", " ## ## ", "  ###  ", "  ###  ", "  ###  ", " ## ## ", "##   ##"}},
    {'Y', {"##   ##", "##   ##", " ## ## ", "  ###  ", "  ###  ", "  ###  ", "  ###  "}},
    {'Z', {"#######", "    ## ", "   ##  ", "  ##   ", " ##    ", "##     ", "#######"}},
    {'0', {" ##### ", "##  ###", "## # ##", "## # ##", "###  ##", "##   ##", " ##### "}},
    {'1', {"  ###  ", " ####  ", "  ###  ", "  ###  ", "  ###  ", "  ###  ", "#######"}},
    {'2', {" ##### ", "##   ##", "     ##", "  #### ", " ##    ", "##     ", "#######"}},
    {'3', {"###### ", "     ##", "     ##", "  #### ", "     ##", "     ##", "###### "}},
    {'4', {"   ### ", "  #### ", " ## ## ", "##  ## ", "#######", "    ## ", "    ## "}},
    {'5', {"#######", "##     ", "###### ", "     ##", "     ##", "##   ##", " ##### "}},
    {'6', {" ##### ", "##     ", "##     ", "###### ", "##   ##", "##   ##", " ##### "}},
    {'7', {"#######", "##   ##", "    ## ", "   ##  ", "  ##   ", "  ##   ", "  ##   "}},
    {'8', {" ##### ", "##   ##", "##   ##", " ##### ", "##   ##", "##   ##", " ##### "}},
    {'9', {" ##### ", "##   ##", "##   ##", " ######", "     ##", "     ##", " ##### "}},
    {':', {"       ", "  ##   ", "  ##   ", "       ", "  ##   ", "  ##   ", "       "}},
    {'.', {"       ", "       ", "       ", "       ", "       ", "  ##   ", "  ##   "}},
    {',', {"       ", "       ", "       ", "       ", "  ##   ", "  ##   ", " ##    "}},
    {'-', {"       ", "       ", "       ", " ##### ", "       ", "       ", "       "}},
    {'+', {"       ", "  ##   ", "  ##   ", "###### ", "  ##   ", "  ##   ", "       "}},
    {'(', {"   ##  ", "  ##   ", " ##    ", " ##    ", " ##    ", "  ##   ", "   ##  "}},
    {')', {"  ##   ", "   ##  ", "    ## ", "    ## ", "    ## ", "   ##  ", "  ##   "}},
    {'/', {"     ##", "    ## ", "   ##  ", "  ##   ", " ##    ", "##     ", "       "}},
    {'!', {"  ##   ", "  ##   ", "  ##   ", "  ##   ", "  ##   ", "       ", "  ##   "}},
    {'?', {" ##### ", "##   ##", "    ## ", "   ##  ", "   ##  ", "       ", "   ##  "}},
    {'>', {" ##    ", "  ##   ", "   ##  ", "    ## ", "   ##  ", "  ##   ", " ##    "}},
    {'<', {"    ## ", "   ##  ", "  ##   ", " ##    ", "  ##   ", "   ##  ", "    ## "}},
    {'\'', {"  ##   ", "  ##   ", " ##    ", "       ", "       ", "       ", "       "}},
    {'_', {"       ", "       ", "       ", "       ", "       ", "       ", "#######"}},
};

static SDL_Renderer *ren;

static const Glyph *glyph(char c)
{
    unsigned i;
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 32);
    for (i = 0; i < sizeof font / sizeof font[0]; i++)
        if (font[i].c == c)
            return &font[i];
    return NULL;
}

/* text at (x, y), s launcher pixels a font pixel, colour rgb, a dark
   shadow down and right */
static void text(float x, float y, float s, const char *t, Uint8 r, Uint8 g, Uint8 b)
{
    int pass;
    for (pass = 0; pass < 2; pass++) {
        float cx = x;
        const char *p;
        if (pass == 0)
            SDL_SetRenderDrawColor(ren, 0, 0, 16, 255);
        else
            SDL_SetRenderDrawColor(ren, r, g, b, 255);
        for (p = t; *p; p++, cx += 8 * s) {
            const Glyph *gl = glyph(*p);
            int row, col;
            if (!gl)
                continue;
            for (row = 0; row < 7; row++)
                for (col = 0; col < 7; col++)
                    if (gl->rows[row][col] == '#') {
                        SDL_FRect q = {cx + col * s + (pass ? 0 : s), y + row * s + (pass ? 0 : s), s, s};
                        SDL_RenderFillRect(ren, &q);
                    }
        }
    }
}


/* ---- settings ---- */
enum { O_DISPLAY, O_RES, O_LOOK, O_FAR, O_WIDE, O_SPEED, O_VSYNC, NOPT };
typedef struct { const char *name, *key; int n, value; const char *const *labels; } Option;
static const char *const l_display[] = {"WINDOW", "FULLSCREEN"};
static const char *const l_res[] = {"ORIGINAL", "2X", "3X", "4X"};
static const char *const l_look[] = {"OFF", "1 FRAME", "2 FRAMES", "3 FRAMES"};
static const char *const l_onoff[] = {"OFF", "ON"};
static const char *const l_speed[] = {"SMOOTH 60 FPS", "ORIGINAL"};
static Option opt[NOPT] = {
    {"DISPLAY", "display", 2, 0, l_display},
    {"3D RESOLUTION", "hires", 4, 2, l_res},
    {"LOOK-AHEAD", "lookahead", 4, 3, l_look},
    {"FAR CARS IN 3D", "farcars", 2, 1, l_onoff},
    {"WIDE VIEW (EXPERIMENTAL)", "wide", 2, 0, l_onoff},
    {"GAME SPEED", "speed", 2, 0, l_speed},
    {"VSYNC", "vsync", 2, 0, l_onoff},
};
static char rom_path[1024];
static int rom_state;            /* 0 none, 1 checked good, 2 bad */
static char cfg_path[1024], save_dir[1024];

static void mkdirs(const char *path)
{
    char b[1024];
    char *p;
    snprintf(b, sizeof b, "%s", path);
    for (p = b + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(b, 0755);
            *p = '/';
        }
    mkdir(b, 0755);
}

static void paths(void)
{
    const char *x = getenv("XDG_CONFIG_HOME"), *d = getenv("XDG_DATA_HOME"), *h = getenv("HOME");
    char dir[1024];
    if (x && *x)
        snprintf(dir, sizeof dir, "%s/stuntracefx", x);
    else
        snprintf(dir, sizeof dir, "%s/.config/stuntracefx", h ? h : ".");
    mkdirs(dir);
    snprintf(cfg_path, sizeof cfg_path, "%s/launcher.cfg", dir);
    if (d && *d)
        snprintf(save_dir, sizeof save_dir, "%s/Stunt Race FX", d);
    else
        snprintf(save_dir, sizeof save_dir, "%s/.local/share/Stunt Race FX", h ? h : ".");
    mkdirs(save_dir);
}

static void cfg_load(void)
{
    FILE *f = fopen(cfg_path, "r");
    char line[1200];
    int i;
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '='), *nl;
        if (!eq)
            continue;
        *eq = 0;
        if ((nl = strchr(eq + 1, '\n')))
            *nl = 0;
        if (!strcmp(line, "rom"))
            snprintf(rom_path, sizeof rom_path, "%s", eq + 1);
        for (i = 0; i < NOPT; i++)
            if (!strcmp(line, opt[i].key)) {
                int v = atoi(eq + 1);
                if (v >= 0 && v < opt[i].n)
                    opt[i].value = v;
            }
    }
    fclose(f);
}

static void cfg_save(void)
{
    FILE *f = fopen(cfg_path, "w");
    int i;
    if (!f)
        return;
    fprintf(f, "rom=%s\n", rom_path);
    for (i = 0; i < NOPT; i++)
        fprintf(f, "%s=%d\n", opt[i].key, opt[i].value);
    fclose(f);
}

/* ---- the ROM's check (SHA-1 of Stunt Race FX (USA) (Rev 1)) ---- */
static void sha1(const Uint8 *m, size_t n, Uint8 out[20])
{
    Uint32 h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0}, w[80];
    Uint64 bits = (Uint64)n * 8;
    size_t total = ((n + 8) / 64 + 1) * 64, off;
    int i;
    for (off = 0; off < total; off += 64) {
        Uint32 a, b, c, d, e;
        for (i = 0; i < 64; i++) {
            size_t k = off + (size_t)i;
            Uint8 v = k < n ? m[k] : k == n ? 0x80 : k >= total - 8 ? (Uint8)(bits >> (8 * (total - 1 - k))) : 0;
            if (!(i & 3))
                w[i >> 2] = 0;
            w[i >> 2] |= (Uint32)v << (24 - 8 * (i & 3));
        }
        for (i = 16; i < 80; i++) {
            Uint32 x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = x << 1 | x >> 31;
        }
        a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
        for (i = 0; i < 80; i++) {
            Uint32 f, k, t;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            t = (a << 5 | a >> 27) + f + e + k + w[i];
            e = d; d = c; c = b << 30 | b >> 2; b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    for (i = 0; i < 20; i++)
        out[i] = (Uint8)(h[i >> 2] >> (24 - 8 * (i & 3)));
}

static void rom_check(void)
{
    static const Uint8 want[20] = {0x78, 0x1e, 0x37, 0x6f, 0xf4, 0x53, 0xa8, 0x3c, 0x7b, 0xcf,
                                   0x72, 0x33, 0x9a, 0xbe, 0x6b, 0x84, 0x2e, 0xfd, 0x9c, 0x86};
    size_t n = 0;
    Uint8 *b, got[20];
    rom_state = 0;
    if (!rom_path[0])
        return;
    b = (Uint8 *)SDL_LoadFile(rom_path, &n);
    if (!b) {
        rom_state = 2;
        return;
    }
    if (n % 1024 == 512) {
        memmove(b, b + 512, n - 512);
        n -= 512;
    }
    sha1(b, n, got);
    SDL_free(b);
    rom_state = memcmp(got, want, 20) ? 2 : 1;
}

/* the file dialog answers on its own thread */
static SDL_Mutex *pick_lock;
static char picked[1024];
static int picked_new;

static void SDLCALL picked_cb(void *user, const char *const *files, int filter)
{
    (void)user;
    (void)filter;
    if (!files || !files[0])
        return;
    SDL_LockMutex(pick_lock);
    snprintf(picked, sizeof picked, "%s", files[0]);
    picked_new = 1;
    SDL_UnlockMutex(pick_lock);
}

/* ---- the game ---- */
static char base_dir[1024], status[128];
static Uint8 status_r = 255, status_g = 255, status_b = 255;

static void set_status(const char *s, Uint8 r, Uint8 g, Uint8 b)
{
    snprintf(status, sizeof status, "%s", s);
    status_r = r;
    status_g = g;
    status_b = b;
}

static int run_game(SDL_Window *win)
{
    char game[1200], logs[1200];
    pid_t pid;
    int st = 0;
    snprintf(game, sizeof game, "%sstuntracefx", base_dir);
    snprintf(logs, sizeof logs, "%s/logs", save_dir);
    mkdirs(logs);
    pid = fork();
    if (pid == 0) {
        static const char *hires[] = {"0", "2", "3", "4"};
        char look[4];
        setenv("PORT_FULLSCREEN", opt[O_DISPLAY].value ? "1" : "0", 1);
        setenv("PORT_HIRES", hires[opt[O_RES].value], 1);
        snprintf(look, sizeof look, "%d", opt[O_LOOK].value);
        setenv("PORT_LOOKAHEAD", look, 1);
        setenv("PORT_FAR_CARS", opt[O_FAR].value ? "18000" : "4500", 1);
        setenv("PORT_WIDE", opt[O_WIDE].value ? "1" : "0", 1);
        setenv("PORT_SPEED", opt[O_SPEED].value ? "original" : "10", 1);
        setenv("PORT_VSYNC", opt[O_VSYNC].value ? "1" : "0", 1);
        setenv("PORT_LOG_DIR", logs, 1);
        if (chdir(save_dir) != 0) {
            /* (the game writes nothing to its working folder; only a nicety) */
        }
        execl(game, game, "--rom", rom_path, (char *)NULL);
        fprintf(stderr, "launcher: cannot start %s: %s\n", game, strerror(errno));
        _exit(127);
    }
    if (pid < 0) {
        set_status("THE GAME COULD NOT START", 255, 96, 96);
        return 0;
    }
    SDL_HideWindow(win);
    while (waitpid(pid, &st, WNOHANG) == 0) {
        SDL_PumpEvents();
        SDL_Delay(50);
    }
    SDL_ShowWindow(win);
    SDL_RaiseWindow(win);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 127)
        set_status("THE GAME IS MISSING", 255, 96, 96);
    else if (WIFSIGNALED(st))
        set_status("THE GAME STOPPED WITH AN ERROR", 255, 96, 96);
    else
        set_status("", 255, 255, 255);
    return 1;
}

/* ---- the screen ---- */
enum { M_PLAY, M_ROM, M_OPT0, M_CONTROLS = M_OPT0 + NOPT, M_QUIT, NMENU };
static int sel;
static int show_controls;

static float item_y(int i) { return 76.0f + (float)i * 18.0f + (i >= M_OPT0 ? 6.0f : 0.0f) + (i >= M_CONTROLS ? 6.0f : 0.0f); }

static void rom_label(char *out, size_t n)
{
    const char *slash;
    if (!rom_path[0]) {
        snprintf(out, n, "CHOOSE YOUR ROM FILE");
        return;
    }
    slash = strrchr(rom_path, '/');
    slash = slash ? slash + 1 : rom_path;
    if (strlen(slash) > 30)
        snprintf(out, n, "%.27s...", slash);
    else
        snprintf(out, n, "%s", slash);
}

static void draw(SDL_Texture *bg)
{
    int i;
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    if (bg) {
        /* the art over the whole screen, cut to its shape (no stretch), darkened */
        float tw, th, s;
        SDL_FRect d;
        SDL_GetTextureSize(bg, &tw, &th);
        s = SDL_max(LW / tw, LH / th);
        d.w = tw * s;
        d.h = th * s;
        d.x = LW - d.w;          /* the car on the right, the menu over its dark left */
        d.y = (LH - d.h) / 2;
        SDL_RenderTexture(ren, bg, NULL, &d);
    }
    {
        /* a darker band behind the menu */
        /* (fading out to the right over the car) */
        int x;
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        for (x = 0; x < 440; x += 4) {
            SDL_FRect band = {(float)x, 0, 4, LH};
            int a = x < 300 ? 130 : 130 * (440 - x) / 140;
            SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)a);
            SDL_RenderFillRect(ren, &band);
        }
    }
    text(20, 16, 3, "STUNT RACE FX", 255, 214, 0);
    text(22, 46, 1, "NATIVE PC PORT  -  BETA", 230, 64, 64);
    if (show_controls) {
        static const char *rows[] = {
            "CONTROLS", "",
            "STEER       ARROWS / D-PAD / STICK",
            "ACCELERATE  Z / A", "BRAKE       X / B",
            "BOOST       A / X", "VIEW        S / Y",
            "SHIFT       Q W / SHOULDERS",
            "START       RETURN / MENU",
            "SELECT      TAB / VIEW", "",
            "IN GAME:", "F3 SPEED  F4 LOOK-AHEAD  F5 3D RES",
            "F7 FAR CARS  F8 WIDE  ALT+ENTER FULL",
            "QUIT: HOLD ESC / VIEW+MENU (1 SEC)", "",
            "PRESS ANY KEY"};
        for (i = 0; i < (int)(sizeof rows / sizeof rows[0]); i++)
            text(24, 74 + i * 14.0f, 1, rows[i], i == 0 ? 255 : 230, i == 0 ? 214 : 230, i == 0 ? 0 : 230);
        return;
    }
    for (i = 0; i < NMENU; i++) {
        float y = item_y(i);
        int on = i == sel;
        Uint8 r = on ? 255 : 235, g = on ? 214 : 235, b = on ? 0 : 235;
        if (on)
            text(10, y, 1, ">", 255, 214, 0);
        if (i == M_PLAY)
            text(24, y, 1, "PLAY", r, g, b);
        else if (i == M_ROM) {
            char l[64];
            rom_label(l, sizeof l);
            text(24, y, 1, "ROM", r, g, b);
            text(104, y, 1, l, rom_state == 1 ? 120 : rom_state == 2 ? 255 : 160, rom_state == 1 ? 220 : rom_state == 2 ? 96 : 200,
                 rom_state == 1 ? 120 : rom_state == 2 ? 96 : 255);
        } else if (i < M_CONTROLS) {
            const Option *o = &opt[i - M_OPT0];
            char v[48];
            text(24, y, 1, o->name, r, g, b);
            snprintf(v, sizeof v, "< %s >", o->labels[o->value]);
            text(236, y, 1, v, 140, 200, 255);
        } else if (i == M_CONTROLS)
            text(24, y, 1, "CONTROLS", r, g, b);
        else
            text(24, y, 1, "QUIT", r, g, b);
    }
    if (status[0])
        text(24, LH - 34, 1, status, status_r, status_g, status_b);
    text(24, LH - 16, 1, "ARROWS / D-PAD   RETURN / A   ESC / B", 150, 150, 170);
}

static void activate(SDL_Window *win, int dir)
{
    if (sel >= M_OPT0 && sel < M_CONTROLS) {
        Option *o = &opt[sel - M_OPT0];
        o->value = (o->value + o->n + (dir ? dir : 1)) % o->n;
        cfg_save();
        return;
    }
    if (dir)
        return;
    if (sel == M_PLAY) {
        if (rom_state != 1) {
            set_status(rom_path[0] ? "THAT ROM IS NOT STUNT RACE FX (USA) REV 1" : "CHOOSE YOUR ROM FILE FIRST", 255, 96, 96);
            sel = M_ROM;
            return;
        }
        cfg_save();
        run_game(win);
    } else if (sel == M_ROM) {
        static const SDL_DialogFileFilter filters[] = {{"SNES ROM", "sfc;smc"}, {"All files", "*"}};
        SDL_ShowOpenFileDialog(picked_cb, NULL, win, filters, 2, rom_path[0] ? rom_path : NULL, false);
    } else if (sel == M_CONTROLS)
        show_controls = 1;
}

static int menu_at(float x, float y)
{
    int i;
    (void)x;
    for (i = 0; i < NMENU; i++)
        if (y >= item_y(i) - 4 && y < item_y(i) + 12)
            return i;
    return -1;
}

int main(int argc, char **argv)
{
    SDL_Window *win;
    SDL_Texture *bg = NULL;
    const char *bp;
    int running = 1;
    (void)argc;
    (void)argv;
    SDL_SetHint(SDL_HINT_APP_ID, "stuntracefx");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "launcher: SDL could not start: %s\n", SDL_GetError());
        return 1;
    }
    bp = SDL_GetBasePath();
    snprintf(base_dir, sizeof base_dir, "%s", bp ? bp : "./");
    paths();
    cfg_load();
    rom_check();
    pick_lock = SDL_CreateMutex();
    win = SDL_CreateWindow("Stunt Race FX", LW * 2, LH * 2, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    ren = win ? SDL_CreateRenderer(win, NULL) : NULL;
    if (!ren) {
        fprintf(stderr, "launcher: no window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(ren, 1);
    SDL_SetRenderLogicalPresentation(ren, LW, LH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    {
        char p[1200];
        SDL_Surface *s;
        snprintf(p, sizeof p, "%slauncher-bg.bmp", base_dir);
        s = SDL_LoadBMP(p);
        if (!s) {
            snprintf(p, sizeof p, "%s../share/stuntracefx/launcher-bg.bmp", base_dir);
            s = SDL_LoadBMP(p);
        }
        if (s) {
            bg = SDL_CreateTextureFromSurface(ren, s);
            SDL_DestroySurface(s);
            SDL_SetTextureScaleMode(bg, SDL_SCALEMODE_LINEAR);
            SDL_SetTextureColorMod(bg, 110, 110, 110);     /* darkened */
        }
    }
    if (rom_state == 2)
        set_status("THE SAVED ROM IS NOT VALID: CHOOSE AGAIN", 255, 96, 96);
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = 0;
            else if (show_controls && (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                                       e.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
                show_controls = 0;
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                case SDLK_UP: sel = (sel + NMENU - 1) % NMENU; break;
                case SDLK_DOWN: sel = (sel + 1) % NMENU; break;
                case SDLK_LEFT: activate(win, -1); break;
                case SDLK_RIGHT: activate(win, 1); break;
                case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE:
                    if (sel == M_QUIT)
                        running = 0;
                    else
                        activate(win, 0);
                    break;
                case SDLK_ESCAPE: running = 0; break;
                }
            } else if (e.type == SDL_EVENT_GAMEPAD_ADDED)
                SDL_OpenGamepad(e.gdevice.which);
            else if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP: sel = (sel + NMENU - 1) % NMENU; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN: sel = (sel + 1) % NMENU; break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT: activate(win, -1); break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: activate(win, 1); break;
                case SDL_GAMEPAD_BUTTON_SOUTH: case SDL_GAMEPAD_BUTTON_START:
                    if (sel == M_QUIT)
                        running = 0;
                    else
                        activate(win, 0);
                    break;
                case SDL_GAMEPAD_BUTTON_EAST: running = 0; break;
                }
            } else if (e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float x = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.x : e.button.x;
                float y = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.y : e.button.y;
                int m;
                SDL_ConvertEventToRenderCoordinates(ren, &e);
                x = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.x : e.button.x;
                y = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.y : e.button.y;
                if (x < 392 && (m = menu_at(x, y)) >= 0) {
                    sel = m;
                    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (sel == M_QUIT)
                            running = 0;
                        else
                            activate(win, e.button.button == SDL_BUTTON_RIGHT ? -1 : 0);
                    }
                }
            }
        }
        SDL_LockMutex(pick_lock);
        if (picked_new) {
            picked_new = 0;
            snprintf(rom_path, sizeof rom_path, "%s", picked);
            rom_check();
            cfg_save();
            if (rom_state == 1)
                set_status("ROM OK: STUNT RACE FX (USA) REV 1", 120, 220, 120);
            else
                set_status("NOT STUNT RACE FX (USA) REV 1", 255, 96, 96);
        }
        SDL_UnlockMutex(pick_lock);
        draw(bg);
        if (getenv("LAUNCHER_SHOT")) {
            /* (a check: one picture saved, then out) */
            SDL_Surface *shot = SDL_RenderReadPixels(ren, NULL);
            if (shot)
                SDL_SaveBMP(shot, getenv("LAUNCHER_SHOT"));
            running = 0;
        }
        SDL_RenderPresent(ren);
    }
    cfg_save();
    SDL_Quit();
    return 0;
}
