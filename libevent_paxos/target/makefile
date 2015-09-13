################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf
export GPROF
# All of the sources participating in the build are defined here
-include sources.mk
-include src/util/subdir.mk
-include src/replica-sys/subdir.mk
-include src/proxy/subdir.mk
-include src/db/subdir.mk
-include src/consensus/subdir.mk
-include src/config-comp/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

LIBS += $(XTERN_ROOT)/obj/lib/libruntime.a -lstdc++ -lm -lboost_system -lpthread -lrt

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: server.out

# Tool invocations
server.out: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker: $(GPROF)'
	#gcc-4.8  -Wall -pg -o   "server.out"  -L"$(ROOT_DIR)/../.local/lib" -pthread  $(OBJS) $(USER_OBJS) $(LIBS)
	gcc-4.8 -Wall $(GPROF) -o "server.out" -L"$(ROOT_DIR)/../.local/lib" -pthread  $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) server.out
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
