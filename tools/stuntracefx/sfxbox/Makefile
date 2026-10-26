# sfxbox: the original ROM run headless on the pinned core, driven by a script.
TOP := ../../..
CORE_SRC := $(TOP)/tools/toolchains/src/bsnes/bsnes
OUT := $(TOP)/build/stuntracefx/box/sfxbox

$(OUT): sfxbox.c check.c check.h gsucall.c gsucall.h spccall.c spccall.h $(TOP)/ports/common/snes/gsu_clock.c $(TOP)/ports/common/snes/gsu_clock.h $(TOP)/src/stuntracefx/include/machine.h $(TOP)/src/stuntracefx/include/gsu.h $(TOP)/src/stuntracefx/include/spc.h
	@mkdir -p $(dir $(OUT))
	cc -O2 -Wall -D_GNU_SOURCE -include stdint.h -I$(CORE_SRC)/target-libretro -I$(TOP)/src/stuntracefx/include -I$(TOP)/ports/common/snes -rdynamic \
		-o $(OUT) sfxbox.c check.c gsucall.c spccall.c $(TOP)/ports/common/snes/gsu_clock.c -ldl
