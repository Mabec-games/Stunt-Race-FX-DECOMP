/*
 * rr_metal.m: the renderer backend on Metal (macOS), the Boss Rally port's
 * design: the frame is drawn into an offscreen target the size of the
 * window's drawable, then copied into the window's CAMetalLayer. Pictures
 * go on with the sharp bilinear shader (rr_size.h's weight, in the shader:
 * fwidth gives the texels to one target pixel). The shader source is
 * compiled at start-up, so the build needs no Metal toolchain step.
 */
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rr.h"
#include "rr_size.h"
#include "host.h"
#include "compose_src.h"
#include "fill_src.h"

CAMetalLayer *host_macos_metal_layer(void);
void host_macos_layer_fit(int *w, int *h);
/* without a window host (HOST=null) there is no layer: frames are drawn
   offscreen and read back, never shown */
__attribute__((weak)) CAMetalLayer *host_macos_metal_layer(void) { return nil; }
__attribute__((weak)) void host_macos_layer_fit(int *w, int *h) { *w = *h = 0; }

static NSString *const k_src = @
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct BO { float4 pos [[position]]; float2 uv; };\n"
"vertex BO bvs(uint i [[vertex_id]], const device float4 *v [[buffer(0)]]) { BO o; o.pos = float4(v[i].xy, 0, 1); o.uv = v[i].zw; return o; }\n"
/* one to one */
"fragment float4 bfs(BO in [[stage_in]], texture2d<float> t [[texture(0)]]) {\n"
"  constexpr sampler s(filter::nearest, address::clamp_to_edge);\n"
"  return float4(t.sample(s, in.uv).rgb, 1); }\n"
/* sharp bilinear: whole texels at any scale, blended only across the one
   target pixel a texel edge falls in */
"fragment float4 sfs(BO in [[stage_in]], texture2d<float> t [[texture(0)]]) {\n"
"  constexpr sampler s(filter::linear, address::clamp_to_edge);\n"
"  float2 ts = float2(t.get_width(), t.get_height()), px = in.uv * ts - 0.5;\n"
"  float2 i = floor(px), f = px - i, d = max(fwidth(px), float2(1e-5));\n"
"  f = clamp((f - 0.5) / d + 0.5, 0.0, 1.0);\n"
"  return float4(t.sample(s, (i + 0.5 + f) / ts).rgb, 1); }\n";

static id<MTLDevice> s_dev;
static id<MTLCommandQueue> s_q;
static id<MTLRenderPipelineState> s_copy, s_sharp;
static id<MTLTexture> s_col;            /* the target */
static id<MTLTexture> s_pic[3];          /* each frame's picture (frames in flight) */
static int s_pi;
static id<MTLBuffer> s_vb[3];
static size_t s_vused;
static id<MTLCommandBuffer> s_cmd;
static id<MTLRenderCommandEncoder> s_enc;
static dispatch_semaphore_t s_inflight;
static int s_rw, s_rh;
static uint32_t s_clear;
/* the compositor: its pipeline, each frame-in-flight's record textures and
   the composed picture */
static id<MTLRenderPipelineState> s_compose;
static id<MTLTexture> s_cvram[3], s_cpal[3], s_coam[3], s_cline[3], s_cpic[3], s_cext[3];
/* the compositor's copy of the tiles' bytes that come from the 3D canvas
   (rr_pack writes it), its pipeline, the runs and their commands */
static id<MTLTexture> s_mirror, s_mirror_kept;   /* the copy packed into; its copies kept, a frame's versions */
static id<MTLRenderPipelineState> s_pack;
static id<MTLBuffer> s_runs;
static id<MTLCommandBuffer> s_pack_cmd;
/* the 3D canvas: its fill's pipeline, the ROM, the canvas, the spans and
   the last fill's commands */
static id<MTLRenderPipelineState> s_fill;
static id<MTLTexture> s_rom, s_canvas[RR_CANVASES];
static id<MTLBuffer> s_spans;
static id<MTLCommandBuffer> s_fill_cmd;
#define VB_SIZE (1u << 16)

static id<MTLRenderPipelineState> make_pipe(NSString *fs, id<MTLLibrary> lib)
{
    MTLRenderPipelineDescriptor *d = [MTLRenderPipelineDescriptor new];
    NSError *err = nil;
    id<MTLRenderPipelineState> p;
    d.vertexFunction = [lib newFunctionWithName:@"bvs"];
    d.fragmentFunction = [lib newFunctionWithName:fs];
    d.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    p = [s_dev newRenderPipelineStateWithDescriptor:d error:&err];
    if (!p)
        fprintf(stderr, "rr: metal pipeline %s: %s\n", [fs UTF8String], [[err description] UTF8String]);
    return p;
}

static id<MTLTexture> int_texture(MTLPixelFormat f, int w, int h, int target)
{
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f width:(NSUInteger)w
                                                                                  height:(NSUInteger)h mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead | (target ? MTLTextureUsageRenderTarget : 0);
    td.storageMode = target ? MTLStorageModePrivate : MTLStorageModeShared;
    return [s_dev newTextureWithDescriptor:td];
}

static void targets(int w, int h)
{
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                   width:(NSUInteger)w height:(NSUInteger)h mipmapped:NO];
    td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    td.storageMode = MTLStorageModeShared;
    s_col = [s_dev newTextureWithDescriptor:td];
    s_rw = w;
    s_rh = h;
}

int rr_open(void)
{
    @autoreleasepool {
        NSError *err = nil;
        id<MTLLibrary> lib;
        CAMetalLayer *ml;
        int i, w, h;
        s_dev = MTLCreateSystemDefaultDevice();
        if (!s_dev)
            return 0;
        s_q = [s_dev newCommandQueue];
        lib = [s_dev newLibraryWithSource:k_src options:nil error:&err];
        if (!lib) {
            fprintf(stderr, "rr: metal shader: %s\n", [[err description] UTF8String]);
            return 0;
        }
        {
            id<MTLLibrary> cl = [s_dev newLibraryWithSource:[NSString stringWithUTF8String:k_compose_msl] options:nil error:&err];
            MTLRenderPipelineDescriptor *d = [MTLRenderPipelineDescriptor new];
            if (!cl) {
                fprintf(stderr, "rr: metal compositor: %s\n", [[err description] UTF8String]);
                return 0;
            }
            d.vertexFunction = [cl newFunctionWithName:@"compose_vs"];
            d.fragmentFunction = [cl newFunctionWithName:@"compose_fs"];
            d.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
            s_compose = [s_dev newRenderPipelineStateWithDescriptor:d error:&err];
            if (!s_compose) {
                fprintf(stderr, "rr: metal compositor pipeline: %s\n", [[err description] UTF8String]);
                return 0;
            }
            for (i = 0; i < 3; i++) {
                s_cvram[i] = int_texture(MTLPixelFormatR16Uint, 256, 128 * RR_VERSIONS, 0);
                s_cpal[i] = int_texture(MTLPixelFormatR16Uint, 256, 225, 0);
                s_coam[i] = int_texture(MTLPixelFormatR8Uint, 544, RR_VERSIONS, 0);
                s_cline[i] = int_texture(MTLPixelFormatR32Uint, RR_LINE_WORDS, 225, 0);
                s_cext[i] = int_texture(MTLPixelFormatR32Uint, 256, 8 * RR_VERSIONS, 0);
                s_cpic[i] = int_texture(MTLPixelFormatBGRA8Unorm, 256, 224, 1);
            }
        }
        {
            id<MTLLibrary> fl = [s_dev newLibraryWithSource:[NSString stringWithUTF8String:k_fill_msl] options:nil error:&err];
            MTLRenderPipelineDescriptor *d = [MTLRenderPipelineDescriptor new];
            if (!fl) {
                fprintf(stderr, "rr: metal fill: %s\n", [[err description] UTF8String]);
                return 0;
            }
            d.vertexFunction = [fl newFunctionWithName:@"fill_vs"];
            d.fragmentFunction = [fl newFunctionWithName:@"fill_fs"];
            d.colorAttachments[0].pixelFormat = MTLPixelFormatR16Uint;
            s_fill = [s_dev newRenderPipelineStateWithDescriptor:d error:&err];
            if (!s_fill) {
                fprintf(stderr, "rr: metal fill pipeline: %s\n", [[err description] UTF8String]);
                return 0;
            }
            fl = [s_dev newLibraryWithSource:[NSString stringWithUTF8String:k_pack_msl] options:nil error:&err];
            if (!fl) {
                fprintf(stderr, "rr: metal pack: %s\n", [[err description] UTF8String]);
                return 0;
            }
            d = [MTLRenderPipelineDescriptor new];
            d.vertexFunction = [fl newFunctionWithName:@"pack_vs"];
            d.fragmentFunction = [fl newFunctionWithName:@"pack_fs"];
            d.colorAttachments[0].pixelFormat = MTLPixelFormatR8Uint;
            s_pack = [s_dev newRenderPipelineStateWithDescriptor:d error:&err];
            if (!s_pack) {
                fprintf(stderr, "rr: metal pack pipeline: %s\n", [[err description] UTF8String]);
                return 0;
            }
            s_mirror = int_texture(MTLPixelFormatR8Uint, 256, 256, 1);
            s_mirror_kept = int_texture(MTLPixelFormatR8Uint, 256, 256 * RR_VERSIONS, 0);
            for (i = 0; i < RR_CANVASES; i++) {
                MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR16Uint
                                                                                               width:256 height:256 mipmapped:NO];
                td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
                td.storageMode = MTLStorageModeShared;
                s_canvas[i] = [s_dev newTextureWithDescriptor:td];
            }
        }
        s_copy = make_pipe(@"bfs", lib);
        s_sharp = make_pipe(@"sfs", lib);
        if (!s_copy || !s_sharp)
            return 0;
        for (i = 0; i < 3; i++)
            s_vb[i] = [s_dev newBufferWithLength:VB_SIZE options:MTLResourceStorageModeShared];
        s_inflight = dispatch_semaphore_create(3);
        ml = host_macos_metal_layer();
        if (ml) {
            [ml setDevice:s_dev];
            [ml setMaximumDrawableCount:3];
            host_macos_layer_fit(&w, &h);
        }
        if (!ml || w < 1 || h < 1)
            rr_headless_size(&w, &h);
        targets(w, h);
    }
    return 1;
}

void rr_close(void) {}

const char *rr_name(void) { return "metal"; }

void rr_target(int *w, int *h)
{
    CAMetalLayer *ml = host_macos_metal_layer();
    if (ml && !s_cmd) {                    /* the next frame at the window's size */
        int fw, fh;
        host_macos_layer_fit(&fw, &fh);
        if (fw > 0 && fh > 0 && (fw != s_rw || fh != s_rh))
            targets(fw, fh);
    }
    *w = s_rw;
    *h = s_rh;
}

/* a quad over target rectangle d, uv 0..1, into this frame's vertex buffer:
   its offset, or -1 */
static long quad(const int d[4])
{
    static const int ix[6] = { 0, 1, 2, 2, 1, 3 };
    float c[4][4] = { { (float)d[0], (float)d[1], 0, 0 }, { (float)d[2], (float)d[1], 1, 0 },
                      { (float)d[0], (float)d[3], 0, 1 }, { (float)d[2], (float)d[3], 1, 1 } };
    float *q;
    size_t o = (s_vused + 15) & ~(size_t)15;
    int i;
    if (o + 6 * 16 > VB_SIZE)
        return -1;
    s_vused = o + 6 * 16;
    q = (float *)((char *)[s_vb[s_pi] contents] + o);
    for (i = 0; i < 6; i++) {
        q[4 * i + 0] = c[ix[i]][0] / s_rw * 2 - 1;
        q[4 * i + 1] = 1 - c[ix[i]][1] / s_rh * 2;
        q[4 * i + 2] = c[ix[i]][2];
        q[4 * i + 3] = c[ix[i]][3];
    }
    return (long)o;
}

void rr_begin(uint32_t argb)
{
    @autoreleasepool {
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        int w, h;
        rr_target(&w, &h);
        dispatch_semaphore_wait(s_inflight, DISPATCH_TIME_FOREVER);
        s_cmd = [s_q commandBuffer];
        {
            dispatch_semaphore_t sem = s_inflight;
            [s_cmd addCompletedHandler:^(id<MTLCommandBuffer> b) { (void)b; dispatch_semaphore_signal(sem); }];
        }
        s_pi = (s_pi + 1) % 3;
        s_vused = 0;
        s_clear = argb;
        rp.colorAttachments[0].texture = s_col;
        rp.colorAttachments[0].loadAction = MTLLoadActionClear;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(((argb >> 16) & 0xFF) / 255.0, ((argb >> 8) & 0xFF) / 255.0,
                                                              (argb & 0xFF) / 255.0, 1);
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        s_enc = [s_cmd renderCommandEncoderWithDescriptor:rp];
    }
}

void rr_image(const uint32_t *argb, int w, int h, const int d[4])
{
    @autoreleasepool {
        id<MTLTexture> t = s_pic[s_pi];
        long off;
        if (!s_enc || w <= 0 || h <= 0 || d[2] <= d[0] || d[3] <= d[1])
            return;
        if (!t || (int)t.width != w || (int)t.height != h) {
            MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                           width:(NSUInteger)w height:(NSUInteger)h mipmapped:NO];
            td.usage = MTLTextureUsageShaderRead;
            t = s_pic[s_pi] = [s_dev newTextureWithDescriptor:td];
        }
        /* 0xAARRGGBB little-endian is BGRA in memory */
        [t replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)w, (NSUInteger)h) mipmapLevel:0
               withBytes:argb bytesPerRow:(NSUInteger)w * 4];
        if ((off = quad(d)) < 0)
            return;
        [s_enc setRenderPipelineState:d[2] - d[0] == w && d[3] - d[1] == h ? s_copy : s_sharp];
        [s_enc setVertexBuffer:s_vb[s_pi] offset:(NSUInteger)off atIndex:0];
        [s_enc setFragmentTexture:t atIndex:0];
        [s_enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

/* the target's pass again, after a pass of another's */
static void resume_target(void)
{
    MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
    rp.colorAttachments[0].texture = s_col;
    rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
    rp.colorAttachments[0].storeAction = MTLStoreActionStore;
    s_enc = [s_cmd renderCommandEncoderWithDescriptor:rp];
}

void rr_compose(const RrPicture *p, const int d[4])
{
    @autoreleasepool {
        int k = s_pi, v = p->versions < 1 ? 1 : p->versions > RR_VERSIONS ? RR_VERSIONS : p->versions;
        long off;
        MTLRenderPassDescriptor *rp;
        id<MTLRenderCommandEncoder> e;
        if (!s_enc || d[2] <= d[0] || d[3] <= d[1])
            return;
        [s_cvram[k] replaceRegion:MTLRegionMake2D(0, 0, 256, (NSUInteger)(128 * v)) mipmapLevel:0 withBytes:p->vram bytesPerRow:512];
        [s_cpal[k] replaceRegion:MTLRegionMake2D(0, 0, 256, 225) mipmapLevel:0 withBytes:p->palette bytesPerRow:512];
        [s_coam[k] replaceRegion:MTLRegionMake2D(0, 0, 544, (NSUInteger)v) mipmapLevel:0 withBytes:p->oam bytesPerRow:544];
        [s_cline[k] replaceRegion:MTLRegionMake2D(0, 0, RR_LINE_WORDS, 225) mipmapLevel:0 withBytes:p->lines
                      bytesPerRow:RR_LINE_WORDS * 4];
        [s_cext[k] replaceRegion:MTLRegionMake2D(0, 0, 256, (NSUInteger)(8 * v)) mipmapLevel:0 withBytes:p->external
                     bytesPerRow:1024];
        [s_enc endEncoding];
        rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = s_cpic[k];
        rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        e = [s_cmd renderCommandEncoderWithDescriptor:rp];
        [e setRenderPipelineState:s_compose];
        [e setFragmentTexture:s_cvram[k] atIndex:0];
        [e setFragmentTexture:s_cpal[k] atIndex:1];
        [e setFragmentTexture:s_coam[k] atIndex:2];
        [e setFragmentTexture:s_cline[k] atIndex:3];
        [e setFragmentTexture:s_cext[k] atIndex:4];
        [e setFragmentTexture:s_mirror_kept atIndex:5];
        [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [e endEncoding];
        resume_target();
        if ((off = quad(d)) < 0)
            return;
        [s_enc setRenderPipelineState:d[2] - d[0] == 256 && d[3] - d[1] == 224 ? s_copy : s_sharp];
        [s_enc setVertexBuffer:s_vb[s_pi] offset:(NSUInteger)off atIndex:0];
        [s_enc setFragmentTexture:s_cpic[k] atIndex:0];
        [s_enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

int rr_fill_rom(const uint8_t *rom, uint32_t size)
{
    @autoreleasepool {
        if (!s_dev || size < 1024 || size % 1024)
            return 0;
        s_rom = int_texture(MTLPixelFormatR8Uint, 1024, (int)(size / 1024), 0);
        [s_rom replaceRegion:MTLRegionMake2D(0, 0, 1024, size / 1024) mipmapLevel:0 withBytes:rom bytesPerRow:1024];
        return 1;
    }
}

void rr_fill(const uint32_t *spans, int n, int clear, int canvas)
{
    @autoreleasepool {
        size_t bytes = (size_t)(n > 0 ? n : 1) * RR_SPAN_WORDS * 4;
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        id<MTLRenderCommandEncoder> e;
        if (!s_rom)
            return;
        if (s_fill_cmd)                        /* the spans' buffer is free again */
            [s_fill_cmd waitUntilCompleted];
        if (!s_spans || [s_spans length] < bytes)
            s_spans = [s_dev newBufferWithLength:bytes * 2 options:MTLResourceStorageModeShared];
        if (n > 0)
            memcpy([s_spans contents], spans, (size_t)n * RR_SPAN_WORDS * 4);
        s_fill_cmd = [s_q commandBuffer];
        rp.colorAttachments[0].texture = s_canvas[canvas];
        rp.colorAttachments[0].loadAction = clear ? MTLLoadActionClear : MTLLoadActionLoad;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        e = [s_fill_cmd renderCommandEncoderWithDescriptor:rp];
        if (n > 0) {
            [e setRenderPipelineState:s_fill];
            [e setVertexBuffer:s_spans offset:0 atIndex:0];
            [e setFragmentTexture:s_rom atIndex:0];
            [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:12 instanceCount:(NSUInteger)n];
        }
        [e endEncoding];
        [s_fill_cmd commit];
    }
}

void rr_pack(const uint32_t *runs, int n, int canvas)
{
    @autoreleasepool {
        size_t bytes = (size_t)n * RR_RUN_WORDS * 4;
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        id<MTLRenderCommandEncoder> e;
        if (!s_pack || n <= 0)
            return;
        if (s_pack_cmd)                        /* the runs' buffer is free again */
            [s_pack_cmd waitUntilCompleted];
        if (!s_runs || [s_runs length] < bytes)
            s_runs = [s_dev newBufferWithLength:bytes * 2 options:MTLResourceStorageModeShared];
        memcpy([s_runs contents], runs, bytes);
        s_pack_cmd = [s_q commandBuffer];
        rp.colorAttachments[0].texture = s_mirror;
        rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        e = [s_pack_cmd renderCommandEncoderWithDescriptor:rp];
        [e setRenderPipelineState:s_pack];
        [e setVertexBuffer:s_runs offset:0 atIndex:0];
        [e setFragmentTexture:s_canvas[canvas] atIndex:0];
        [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:(NSUInteger)n];
        [e endEncoding];
        [s_pack_cmd commit];
    }
}

void rr_pack_keep(int version)
{
    @autoreleasepool {
        id<MTLCommandBuffer> c;
        id<MTLBlitCommandEncoder> b;
        if (!s_mirror || version < 0 || version >= RR_VERSIONS)
            return;
        c = [s_q commandBuffer];
        b = [c blitCommandEncoder];
        [b copyFromTexture:s_mirror sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
                sourceSize:MTLSizeMake(256, 256, 1) toTexture:s_mirror_kept destinationSlice:0 destinationLevel:0
         destinationOrigin:MTLOriginMake(0, (NSUInteger)(256 * version), 0)];
        [b endEncoding];
        [c commit];
    }
}

int rr_pack_read(uint8_t *bytes)
{
    @autoreleasepool {
        id<MTLTexture> t;
        id<MTLCommandBuffer> c;
        id<MTLBlitCommandEncoder> b;
        if (!s_mirror)
            return 0;
        t = int_texture(MTLPixelFormatR8Uint, 256, 256, 0);
        c = [s_q commandBuffer];
        b = [c blitCommandEncoder];
        [b copyFromTexture:s_mirror toTexture:t];
        [b endEncoding];
        [c commit];
        [c waitUntilCompleted];
        [t getBytes:bytes bytesPerRow:256 fromRegion:MTLRegionMake2D(0, 0, 256, 256) mipmapLevel:0];
        return 1;
    }
}

int rr_fill_read(uint16_t *canvas)
{
    @autoreleasepool {
        if (!s_fill_cmd)
            return 0;
        [s_fill_cmd waitUntilCompleted];
        [s_canvas[0] getBytes:canvas bytesPerRow:512 fromRegion:MTLRegionMake2D(0, 0, 256, 256) mipmapLevel:0];
        return 1;
    }
}

void rr_present(void)
{
    @autoreleasepool {
        CAMetalLayer *ml = host_window_visible() ? host_macos_metal_layer() : nil;
        if (!s_enc)
            return;
        [s_enc endEncoding];
        s_enc = nil;
        if (ml) {
            id<CAMetalDrawable> dr = [ml nextDrawable];
            if (dr && (int)[dr texture].width == s_rw && (int)[dr texture].height == s_rh) {
                id<MTLBlitCommandEncoder> b = [s_cmd blitCommandEncoder];
                [b copyFromTexture:s_col sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
                        sourceSize:MTLSizeMake((NSUInteger)s_rw, (NSUInteger)s_rh, 1)
                         toTexture:[dr texture] destinationSlice:0 destinationLevel:0
                 destinationOrigin:MTLOriginMake(0, 0, 0)];
                [b endEncoding];
                [s_cmd presentDrawable:dr];
            }
        }
        [s_cmd commit];
        s_cmd = nil;
    }
}

int rr_read(uint32_t *argb)
{
    @autoreleasepool {
        id<MTLCommandBuffer> c = [s_q commandBuffer];
        [c commit];
        [c waitUntilCompleted];                /* everything before it is done */
        [s_col getBytes:argb bytesPerRow:(NSUInteger)s_rw * 4
             fromRegion:MTLRegionMake2D(0, 0, (NSUInteger)s_rw, (NSUInteger)s_rh) mipmapLevel:0];
        return 1;
    }
}
