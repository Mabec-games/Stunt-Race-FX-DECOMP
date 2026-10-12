# sfxbox: the original ROM run headless on the pinned core, driven by a script.
TOP := ../../..
CORE_SRC := $(TOP)/tools/toolchains/src/bsnes/bsnes
OUT := $(TOP)/build/stuntracefx/box/sfxbox

$(OUT): sfxbox.c check.c check.h $(TOP)/src/stuntracefx/include/machine.h
	@mkdir -p $(dir $(OUT))
	cc -O2 -Wall -I$(CORE_SRC)/target-libretro -I$(TOP)/src/stuntracefx/include -Wl,-export_dynamic \
		-o $(OUT) sfxbox.c check.c -ldl
