#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make.
#

# Program version
VERSIONSTR=pre-0.5.3
ifdef BUILD_NUMBER
    VERSION=$(VERSIONSTR)-build$(BUILD_NUMBER)
else
    VERSION=$(VERSIONSTR)-$(shell date +"%Y%m%d_%H%M%S")
endif

# Output directory
OUT=out/

# Default compiler flags (note -march=armv4 is needed for 16 bit insns)
CXXFLAGS_ARMV4 = -Wall -O -g -MD -march=armv4 -Iinclude -fno-exceptions -fno-rtti
CXXFLAGS_ARMV5 = -Wall -O -g -MD -march=armv5 -Iinclude -fno-exceptions -fno-rtti
CXXFLAGS_ARMV6 = -Wall -O -g -MD -march=armv6 -Iinclude -fno-exceptions -fno-rtti
LDFLAGS = -Wl,--major-subsystem-version=2,--minor-subsystem-version=10 -static-libgcc
# LDFLAGS to debug invalid imports in exe
#LDFLAGS = -Wl,-M -Wl,--cref

LIBS = -lwinsock -lKMDriverWrapper -L$(OUT)obj/

all: $(OUT) $(OUT)Windows/kmodedll.dll $(OUT)obj/libKMDriverWrapper.a $(OUT)haret.exe $(OUT)haretconsole.tar.gz $(OUT)Windows/KMDriver.dll $(OUT)Windows/KMDriverWrapper.dll $(OUT)KMDriver.reg

# Run with "make V=1" to see the actual compile commands
ifdef V
Q=
else
Q=@
endif

.PHONY : all FORCE

vpath %.cpp src src/wince src/mach
vpath %.S src src/wince src/mach
vpath %.rc src/wince

################ cegcc settings

BASE ?= /opt/mingw32ce
export BASE

#Try to see if g++ is found from BASE if not use PATH

GPPOUTTEST := $(shell $(BASE)/bin/arm-mingw32ce-g++ 2>&1)
ifeq (arm-mingw32ce-g++: no input files,$(GPPOUTTEST))
  GCCBINDIR = $(BASE)/bin/
else
  GCCBINDIR =
endif
export GCCBINDIR

RC = $(GCCBINDIR)arm-mingw32ce-windres
RCFLAGS = -r -l 0x409 -Iinclude

CXX = $(GCCBINDIR)arm-mingw32ce-g++
STRIP = $(GCCBINDIR)arm-mingw32ce-strip

DLLTOOL = $(GCCBINDIR)arm-mingw32ce-dlltool
DLLTOOLFLAGS =

define compile_armv4
@echo "  Compiling (armv4) $1"
$(Q)$(CXX) $(CXXFLAGS_ARMV4) -c $1 -o $2
endef

define compile_armv5
@echo "  Compiling (armv5) $1"
$(Q)$(CXX) $(CXXFLAGS_ARMV5) -c $1 -o $2
endef

define compile_armv6
@echo "  Compiling (armv6) $1"
$(Q)$(CXX) $(CXXFLAGS_ARMV6) -c $1 -o $2
endef

define compile
$(if $(findstring -armv6.,$1),
	$(call compile_armv6,$1,$2),
	$(if $(findstring -armv5.,$1),
		$(call compile_armv5,$1,$2),
		$(call compile_armv4,$1,$2)
	)
)
endef

$(OUT)obj/%.o: %.cpp ; $(call compile,$<,$@)
$(OUT)obj/%.o: %.S ; $(call compile,$<,$@)

$(OUT)obj/%.o: %.rc
	@echo "  Building resource file from $<"
	$(Q)$(RC) $(RCFLAGS) -i $< -o $@

$(OUT)obj/%.lib: src/wince/%.def
	@echo "  Building library $@"
	$(Q)$(DLLTOOL) $(DLLTOOLFLAGS) -d $< -l $@

$(OUT)%-debug:
	$(Q)echo 'const char *VERSION = "$(VERSION)";' > $(OUT)obj/version.cpp
	$(call compile,$(OUT)obj/version.cpp,$(OUT)obj/version.o)
	@echo "  Checking for relocations"
	$(Q)tools/checkrelocs $(filter %.o,$^)
	@echo "  Linking $@ (Version \"$(VERSION)\")"
	$(Q)$(CXX) $(LDFLAGS) $(OUT)obj/version.o $^ $(LIBS) -o $@

$(OUT)%.exe: $(OUT)%-debug
	@echo "  Stripping $^ to make $@"
	$(Q)$(STRIP) $^ -o $@

################ Haret exe rules

# List of machines supported - note order is important - it determines
# which machines are checked first.
MACHOBJS := machines.o \
  mach-autogen.o \
  arch-pxa27x.o arch-pxa.o arch-sa.o arch-omap.o arch-s3.o arch-msm.o \
  arch-imx.o arch-centrality.o arch-arm.o arch-msm-asm.o

$(OUT)obj/mach-autogen.o: src/mach/machlist.txt
	@echo "  Building machine list"
	$(Q)tools/buildmachs.py < $^ > $(OUT)obj/mach-autogen.cpp
	$(call compile,$(OUT)obj/mach-autogen.cpp,$@)

COREOBJS := $(MACHOBJS) haret-res.o libcfunc.o \
  script.o memory.o video.o asmstuff.o lateload.o output.o cpu.o \
  linboot.o fbwrite.o font_mini_4x6.o winvectors.o exceptions.o \
  asmstuff-armv5.o

HARETOBJS := $(COREOBJS) haret.o gpio.o uart.o wincmds.o \
  watch.o irqchain.o irq.o pxatrace.o mmumerge.o l1trace.o arminsns.o \
  network.o terminal.o com_port.o tlhcmds.o memcmds.o pxacmds.o aticmds.o \
  imxcmds.o s3c-gpio.o msmcmds.o kernelmodestuff.o

$(OUT)haret-debug: $(addprefix $(OUT)obj/,$(HARETOBJS)) src/haret.lds

################ Kernel mode dll rules

$(OUT)Windows/kmodedll.dll: src/kmodedll/*.*
	$(call compile,src/kmodedll/kmode_dll.cpp,$(OUT)obj/kmode_dll.o -DBUILDING_KMODE_DLL)
	@echo "Linking kmodedll.dll"
	$(Q)$(CXX) $(LDFLAGS) -s -shared -o $(OUT)Windows/kmodedll.dll $(OUT)obj/kmode_dll.o -Wl,--out-implib,$(OUT)obj/libkmodedll.a
	
$(OUT)obj/libKMDriverWrapper.a: lib/KMDriverWrapper.def
	$(Q)$(DLLTOOL) $(DLLTOOLFLAGS) -D KMDriverWrapper.dll -d lib/KMDriverWrapper.def -l $(OUT)obj/libKMDriverWrapper.a

$(OUT)Windows/KMDriver.dll: lib/KMDriver.dll
	$(Q)cp $< $@
$(OUT)Windows/KMDriverWrapper.dll: lib/KMDriverWrapper.dll
	$(Q)cp $< $@
$(OUT)KMDriver.reg: lib/KMDriver.reg
	$(Q)cp $< $@

####### Stripped down linux bootloading program.
LINLOADOBJS := $(COREOBJS) stubboot.o kernelfiles.o

KERNEL := zImage
INITRD := /dev/null
SCRIPT := docs/linload.txt

$(OUT)obj/kernelfiles.o: src/wince/kernelfiles.S FORCE
	@echo "  Building $@"
	$(Q)$(CXX) -c -DLIN_KERNEL=\"$(KERNEL)\" -DLIN_INITRD=\"$(INITRD)\" -DLIN_SCRIPT=\"$(SCRIPT)\" -o $@ $<

$(OUT)oldlinload-debug: $(addprefix $(OUT)obj/, $(LINLOADOBJS)) src/haret.lds

oldlinload: $(OUT)oldlinload.exe

linload: $(OUT) $(OUT)haret.exe
	@echo "  Building boot bundle"
	$(Q)tools/make-bootbundle.py -o $(OUT)linload.exe $(OUT)haret.exe $(KERNEL) $(INITRD) $(SCRIPT)

####### Haretconsole tar files

HC_FILES := README console *.py arm-linux-objdump

$(OUT)haretconsole.tar.gz: $(wildcard $(addprefix haretconsole/, $(HC_FILES)))
	@echo "  Creating tar $@"
	$(Q)tar cfz $@ $^

####### Generic rules
clean:
	@echo "Cleaning output directory $(OUT)"
	$(Q)rm -rf $(OUT)

$(OUT):
	@echo "Generating output directories at $@"
	$(Q)mkdir $@
	$(Q)mkdir $@/obj
	$(Q)mkdir $@/Windows

-include $(OUT)*.d

