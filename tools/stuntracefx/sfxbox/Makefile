# sfxbox: the original ROM run headless on the pinned core, driven by a script.
TOP := ../../..
CORE_SRC := $(TOP)/tools/toolchains/src/bsnes/bsnes
OUT := $(TOP)/build/stuntracefx/box/sfxbox

$(OUT): sfxbox.c
	@mkdir -p $(dir $(OUT))
	cc -O2 -Wall -I$(CORE_SRC)/target-libretro -o $(OUT) sfxbox.c -ldl
