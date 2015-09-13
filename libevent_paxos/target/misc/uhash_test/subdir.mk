################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../misc/uhash_test/main.c 

OBJS += \
./misc/uhash_test/main.o 

C_DEPS += \
./misc/uhash_test/main.d 


# Each subdirectory must supply rules for building sources it contributes
misc/uhash_test/%.o: ../misc/uhash_test/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler: $(GPORF)'
	#gcc-4.8 -std=gnu11 -DDEBUG=1 -I"$(ROOT_DIR)/../.local/include" -O0 -g3 -Wall -pg -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	gcc-4.8 -std=gnu11 -DDEBUG=1 -I"$(ROOT_DIR)/../.local/include" -O0 -g3 -Wall $(GPROF) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


