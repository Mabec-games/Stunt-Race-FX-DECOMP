/* host_macos.m: the macOS host -- a Cocoa window that shows the frames, the
 * keyboard and game controllers, Core Audio out.
 *
 * The game keeps its own loop on the main thread, so this host never runs
 * [NSApp run]: each host_poll_event drains Cocoa's queue itself. Frames
 * arrive as ARGB pixels through host_present and are shown as the view's
 * layer contents, scaled to the window with nearest-neighbour filtering at
 * the shape the game asks for (the window is held at that shape; full
 * screen letterboxes it).
 *
 * PORT_SAVEDIR names the save folder (default ~/Library/Application
 * Support/<app>); PORT_NOACTIVATE=1 opens the window without taking the
 * focus (test runs).
 */
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>
#import <GameController/GameController.h>
#include <sys/stat.h>

#include "host.h"

static char s_save[1024];
static const char *s_app_dir = "Game", *s_app_title = "Game";

void host_set_app_name(const char *dir, const char *title)
{
    if (dir)
        s_app_dir = dir;
    if (title)
        s_app_title = title;
}

/* ---- process -------------------------------------------------------------------- */
void host_init(int argc, char **argv)
{
    const char *e;
    (void)argc;
    (void)argv;
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        {
            NSMenu *bar = [NSMenu new], *app = [NSMenu new];
            NSMenuItem *item = [NSMenuItem new];
            [app addItemWithTitle:[NSString stringWithFormat:@"Quit %s", s_app_title] action:@selector(terminate:) keyEquivalent:@"q"];
            [item setSubmenu:app];
            [bar addItem:item];
            {                                         /* View: full screen (the window's own action) */
                NSMenu *view = [[NSMenu alloc] initWithTitle:@"View"];
                NSMenuItem *vi = [NSMenuItem new], *fs;
                fs = [view addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
                [fs setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
                [vi setSubmenu:view];
                [bar addItem:vi];
            }
            [NSApp setMainMenu:bar];
        }
        [NSApp finishLaunching];

        e = getenv("PORT_SAVEDIR");
        if (e) {
            snprintf(s_save, sizeof s_save, "%s", e);
        } else {
            NSString *base = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                                 NSUserDomainMask, YES) firstObject];
            snprintf(s_save, sizeof s_save, "%s/%s", base ? [base fileSystemRepresentation] : ".", s_app_dir);
        }
        host_mkdir(s_save);
    }
}

const char *host_resource_dir(void)
{
    static char res[1024];
    const char *e = getenv("PORT_RESDIR");
    if (e)
        return e;
    if (!res[0]) {
        @autoreleasepool {
            NSString *r = [[NSBundle mainBundle] resourcePath];
            snprintf(res, sizeof res, "%s", r ? [r fileSystemRepresentation] : ".");
        }
    }
    return res;
}

void host_shutdown(void)
{
    host_audio_close();
    host_window_close();
}

const char *host_save_dir(void) { return s_save; }

/* ---- keys: mac virtual key -> Win32 virtual key ------------------------------------ */
static const struct { uint16_t mac; uint8_t vk; } k_keys[] = {
    { 0x35, 0x1B }, { 0x24, 0x0D }, { 0x4C, 0x0D }, { 0x31, 0x20 }, { 0x30, 0x09 }, { 0x33, 0x08 },
    { 0x7B, 0x25 }, { 0x7C, 0x27 }, { 0x7E, 0x26 }, { 0x7D, 0x28 },
    { 0x38, 0x10 }, { 0x3C, 0x10 }, { 0x3B, 0x11 }, { 0x3E, 0x11 }, { 0x3A, 0x12 }, { 0x3D, 0x12 },
    { 0x00, 'A' }, { 0x0B, 'B' }, { 0x08, 'C' }, { 0x02, 'D' }, { 0x0E, 'E' }, { 0x03, 'F' },
    { 0x05, 'G' }, { 0x04, 'H' }, { 0x22, 'I' }, { 0x26, 'J' }, { 0x28, 'K' }, { 0x25, 'L' },
    { 0x2E, 'M' }, { 0x2D, 'N' }, { 0x1F, 'O' }, { 0x23, 'P' }, { 0x0C, 'Q' }, { 0x0F, 'R' },
    { 0x01, 'S' }, { 0x11, 'T' }, { 0x20, 'U' }, { 0x09, 'V' }, { 0x0D, 'W' }, { 0x07, 'X' },
    { 0x10, 'Y' }, { 0x06, 'Z' },
    { 0x12, '1' }, { 0x13, '2' }, { 0x14, '3' }, { 0x15, '4' }, { 0x17, '5' }, { 0x16, '6' },
    { 0x1A, '7' }, { 0x1C, '8' }, { 0x19, '9' }, { 0x1D, '0' },
    { 0x7A, 0x70 }, { 0x78, 0x71 }, { 0x63, 0x72 }, { 0x76, 0x73 }, { 0x60, 0x74 }, { 0x61, 0x75 },
    { 0x62, 0x76 }, { 0x64, 0x77 }, { 0x65, 0x78 }, { 0x6D, 0x79 }, { 0x67, 0x7A }, { 0x6F, 0x7B },
    { 0x32, 0xC0 }, { 0x1B, 0xBD }, { 0x18, 0xBB }, { 0x21, 0xDB }, { 0x1E, 0xDD }, { 0x2A, 0xDC },
    { 0x29, 0xBA }, { 0x27, 0xDE }, { 0x2B, 0xBC }, { 0x2F, 0xBE }, { 0x2C, 0xBF },
    { 0x73, 0x24 }, { 0x74, 0x21 }, { 0x77, 0x23 }, { 0x79, 0x22 }, { 0x72, 0x2D }, { 0x75, 0x2E },
    { 0x52, 0x60 }, { 0x53, 0x61 }, { 0x54, 0x62 }, { 0x55, 0x63 }, { 0x56, 0x64 }, { 0x57, 0x65 },
    { 0x58, 0x66 }, { 0x59, 0x67 }, { 0x5B, 0x68 }, { 0x5C, 0x69 }, { 0x41, 0x6E }, { 0x43, 0x6A },
    { 0x45, 0x6B }, { 0x4E, 0x6D }, { 0x4B, 0x6F },
};

/* ---- the event queue the host fills and host_poll_event drains ------------------- */
#define QMAX 256
static host_event s_q[QMAX];
static int s_qh, s_qt;
static uint8_t s_down[128];          /* by mac key code: held, for modifier changes */

static void qpush(const host_event *e)
{
    int n = (s_qt + 1) % QMAX;
    if (n != s_qh) {
        s_q[s_qt] = *e;
        s_qt = n;
    }
}

static void key_event(uint16_t mac, int down)
{
    size_t i;
    if (mac < 128) {
        if (s_down[mac] == down)
            return;               /* auto-repeat and modifier duplicates */
        s_down[mac] = (uint8_t)down;
    }
    for (i = 0; i < sizeof k_keys / sizeof k_keys[0]; i++)
        if (k_keys[i].mac == mac) {
            host_event e;
            memset(&e, 0, sizeof e);
            e.type = HOST_EV_KEY;
            e.vk = k_keys[i].vk;
            e.down = down;
            qpush(&e);
            return;
        }
}

/* ---- the window ----------------------------------------------------------------- */
@interface PortView : NSView
@end

@interface PortWindowDelegate : NSObject <NSWindowDelegate>
@end

static NSWindow *s_win;
static PortView *s_view;
static CGColorSpaceRef s_rgb;
static float s_aspect;

@implementation PortView
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)wantsUpdateLayer { return YES; }
- (void)keyDown:(NSEvent *)ev
{
    if (!([ev modifierFlags] & NSEventModifierFlagCommand))
        key_event([ev keyCode], 1);
}
- (void)keyUp:(NSEvent *)ev { key_event([ev keyCode], 0); }
- (void)flagsChanged:(NSEvent *)ev
{
    /* a modifier's own key code: down when its flag is now set */
    NSEventModifierFlags f = [ev modifierFlags];
    uint16_t k = [ev keyCode];
    int down = 0;
    switch (k) {
    case 0x38: case 0x3C: down = (f & NSEventModifierFlagShift) != 0; break;
    case 0x3B: case 0x3E: down = (f & NSEventModifierFlagControl) != 0; break;
    case 0x3A: case 0x3D: down = (f & NSEventModifierFlagOption) != 0; break;
    default: return;
    }
    key_event(k, down);
}
@end

@implementation PortWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)w
{
    host_event e;
    (void)w;
    memset(&e, 0, sizeof e);
    e.type = HOST_EV_CLOSE;
    qpush(&e);
    return NO;                /* the game decides */
}
- (void)windowDidBecomeKey:(NSNotification *)n
{
    host_event e;
    (void)n;
    memset(&e, 0, sizeof e);
    e.type = HOST_EV_FOCUS;
    e.down = 1;
    qpush(&e);
}
- (void)windowDidResignKey:(NSNotification *)n
{
    host_event e;
    size_t i;
    (void)n;
    /* keys held when focus leaves go up, or they stay down for good */
    for (i = 0; i < 128; i++)
        if (s_down[i])
            key_event((uint16_t)i, 0);
    memset(&e, 0, sizeof e);
    e.type = HOST_EV_FOCUS;
    e.down = 0;
    qpush(&e);
}
@end

static PortWindowDelegate *s_delegate;

int host_window_open(int width, int height, const char *title)
{
    @autoreleasepool {
        NSRect r;
        if (s_win)
            return 1;
        r = NSMakeRect(0, 0, width * 3, height * 3);
        s_win = [[NSWindow alloc] initWithContentRect:r
                                            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                                      NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
        [s_win setTitle:[NSString stringWithUTF8String:title ? title : s_app_title]];
        [s_win setContentAspectRatio:NSMakeSize(width, height)];
        [s_win setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
        s_view = [[PortView alloc] initWithFrame:r];
        [s_view setWantsLayer:YES];
        [[s_view layer] setBackgroundColor:CGColorGetConstantColor(kCGColorBlack)];
        [s_win setContentView:s_view];
        s_delegate = [PortWindowDelegate new];
        [s_win setDelegate:s_delegate];
        [s_win center];
        [s_win makeKeyAndOrderFront:nil];
        [s_win makeFirstResponder:s_view];
        if (!getenv("PORT_NOACTIVATE"))       /* a test run launched in the background stays there */
            [NSApp activateIgnoringOtherApps:YES];
        s_rgb = CGColorSpaceCreateDeviceRGB();
    }
    return 1;
}

void host_window_close(void)
{
    if (s_win) {
        [s_win orderOut:nil];
        s_win = nil;
        s_view = nil;
    }
}

int host_window_visible(void)
{
    return s_win && ([s_win occlusionState] & NSWindowOcclusionStateVisible) != 0;
}

/* a picture of ARGB pixels (0xAARRGGBB, top row first) onto the window, at
   the shape `aspect` (width over height): drawn into the largest rectangle
   of that shape the view holds, centred (the window is held at that shape,
   so only full screen shows black bars) */
static CALayer *s_pic;

void host_present(const uint32_t *argb, int w, int h, float aspect)
{
    @autoreleasepool {
        CGDataProviderRef dp;
        CGImageRef img;
        NSData *d;
        NSRect b;
        CGFloat fw, fh;
        if (!s_view || !argb)
            return;
        if (aspect <= 0)
            aspect = (float)w / (float)h;
        if (aspect != s_aspect) {
            NSRect f = [s_view bounds];
            s_aspect = aspect;
            [s_win setContentAspectRatio:NSMakeSize(aspect * 1000, 1000)];
            if (!([s_win styleMask] & NSWindowStyleMaskFullScreen))
                [s_win setContentSize:NSMakeSize(floor(f.size.height * aspect + 0.5), f.size.height)];
        }
        d = [NSData dataWithBytes:argb length:(NSUInteger)w * (NSUInteger)h * 4];
        dp = CGDataProviderCreateWithCFData((__bridge CFDataRef)d);
        img = CGImageCreate((size_t)w, (size_t)h, 8, 32, (size_t)w * 4, s_rgb,
                            kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
                            dp, NULL, false, kCGRenderingIntentDefault);
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        if (!s_pic) {
            s_pic = [CALayer layer];
            [s_pic setMagnificationFilter:kCAFilterNearest];
            [s_pic setContentsGravity:kCAGravityResize];
            [[s_view layer] addSublayer:s_pic];
        }
        b = [s_view bounds];
        fw = b.size.width;
        fh = b.size.height;
        if (fh > 0 && fw / fh > aspect)
            fw = floor(fh * aspect + 0.5);
        else
            fh = floor(fw / aspect + 0.5);
        [s_pic setFrame:CGRectMake(floor((b.size.width - fw) / 2), floor((b.size.height - fh) / 2), fw, fh)];
        [s_pic setContentsScale:[s_win backingScaleFactor]];
        [s_pic setContents:(__bridge id)img];
        [CATransaction commit];
        CGImageRelease(img);
        CGDataProviderRelease(dp);
    }
}

int host_poll_event(host_event *ev, uint32_t wait_ms)
{
    @autoreleasepool {
        NSDate *until = wait_ms == 0 ? [NSDate distantPast]
                      : wait_ms == 0xFFFFFFFFu ? [NSDate distantFuture]
                      : [NSDate dateWithTimeIntervalSinceNow:wait_ms / 1000.0];
        while (s_qh == s_qt) {
            NSEvent *e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until
                                               inMode:NSDefaultRunLoopMode dequeue:YES];
            if (!e)
                break;
            [NSApp sendEvent:e];
            until = [NSDate distantPast];      /* the rest of what is queued, no more waiting */
        }
    }
    if (s_qh == s_qt)
        return 0;
    *ev = s_q[s_qh];
    s_qh = (s_qh + 1) % QMAX;
    return 1;
}

void host_message_box(const char *text, const char *caption)
{
    @autoreleasepool {
        NSAlert *a = [NSAlert new];
        [a setMessageText:[NSString stringWithUTF8String:caption ? caption : s_app_title]];
        [a setInformativeText:[NSString stringWithUTF8String:text ? text : ""]];
        [a runModal];
    }
}

/* ---- audio out: an output unit pulling the game's mixer --------------------------- */
static AudioComponentInstance s_unit;
static host_audio_fn s_afn;
static void *s_auser;

static OSStatus render(void *ref, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts,
                       UInt32 bus, UInt32 frames, AudioBufferList *io)
{
    (void)ref; (void)flags; (void)ts; (void)bus;
    if (s_afn)
        s_afn((float *)io->mBuffers[0].mData, (int)frames, s_auser);
    else
        memset(io->mBuffers[0].mData, 0, io->mBuffers[0].mDataByteSize);
    return noErr;
}

int host_audio_open(int rate, host_audio_fn fn, void *user)
{
    AudioComponentDescription desc = { kAudioUnitType_Output, kAudioUnitSubType_DefaultOutput,
                                       kAudioUnitManufacturer_Apple, 0, 0 };
    AudioStreamBasicDescription fmt;
    AURenderCallbackStruct cb = { render, NULL };
    AudioComponent comp;
    if (s_unit)
        return 1;
    comp = AudioComponentFindNext(NULL, &desc);
    if (!comp || AudioComponentInstanceNew(comp, &s_unit) != noErr)
        return 0;
    memset(&fmt, 0, sizeof fmt);
    fmt.mSampleRate = rate;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    fmt.mFramesPerPacket = 1;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel = 32;
    fmt.mBytesPerFrame = 8;
    fmt.mBytesPerPacket = 8;
    s_afn = fn;
    s_auser = user;
    if (AudioUnitSetProperty(s_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
                             &fmt, sizeof fmt) != noErr ||
        AudioUnitSetProperty(s_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0,
                             &cb, sizeof cb) != noErr ||
        AudioUnitInitialize(s_unit) != noErr || AudioOutputUnitStart(s_unit) != noErr) {
        AudioComponentInstanceDispose(s_unit);
        s_unit = NULL;
        return 0;
    }
    return 1;
}

void host_audio_close(void)
{
    if (s_unit) {
        AudioOutputUnitStop(s_unit);
        AudioUnitUninitialize(s_unit);
        AudioComponentInstanceDispose(s_unit);
        s_unit = NULL;
    }
}

/* ---- game controllers ------------------------------------------------------------- */
/* any controller macOS knows (Xbox, PlayStation, MFi, Switch Pro) through
 * GameController.framework, in the order macOS lists those with a full
 * gamepad profile. One plugged in while the game runs is picked up at the
 * next read. */
int host_pad_read(int index, host_pad *o)
{
    @autoreleasepool {
        GCController *c = nil;
        GCExtendedGamepad *g;
        unsigned b = 0;
        int n = 0;
        memset(o, 0, sizeof *o);
        for (GCController *k in GCController.controllers)
            if (k.extendedGamepad && n++ == index) {
                c = k;
                break;
            }
        if (!c || !(g = c.extendedGamepad))
            return 0;
        o->x = g.leftThumbstick.xAxis.value;
        o->y = -g.leftThumbstick.yAxis.value;
        if (g.buttonA.pressed) b |= 1u << 0;
        if (g.buttonB.pressed) b |= 1u << 1;
        if (g.buttonX.pressed) b |= 1u << 2;
        if (g.buttonY.pressed) b |= 1u << 3;
        if (g.leftShoulder.pressed) b |= 1u << 4;
        if (g.rightShoulder.pressed) b |= 1u << 5;
        if (g.buttonOptions.pressed) b |= 1u << 6;
        if (g.buttonMenu.pressed) b |= 1u << 7;
        if (g.leftThumbstickButton.pressed) b |= 1u << 8;
        if (g.rightThumbstickButton.pressed) b |= 1u << 9;
        if (g.leftTrigger.value > 0.5f) b |= 1u << 10;
        if (g.rightTrigger.value > 0.5f) b |= 1u << 11;
        if (g.dpad.up.pressed) b |= 1u << 12;
        if (g.dpad.right.pressed) b |= 1u << 13;
        if (g.dpad.down.pressed) b |= 1u << 14;
        if (g.dpad.left.pressed) b |= 1u << 15;
        o->buttons = b;
        return 1;
    }
}
