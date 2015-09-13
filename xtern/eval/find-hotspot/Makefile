LEVEL = $(shell $(LLVM_ROOT)/scripts/level-to-llvm-root)/llvm-obj
LIBRARYNAME = find-hotspot
LOADABLE_MODULE = 1
BUILD_ARCHIVE = 1

INSTRUMENT_LIBS = $(LLVM_ROOT)/install/lib/libid-manager.a

include $(LEVEL)/Makefile.common

LIBS += \
	$(INSTRUMENT_LIBS)

CXXFLAGS += -I$(LLVM_ROOT)/common/include/

clean::
	rm -rf Debug
	rm -rf Release
