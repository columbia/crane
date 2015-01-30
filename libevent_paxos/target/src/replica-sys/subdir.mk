################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/replica-sys/message.c \
../src/replica-sys/replica.c 

OBJS += \
./src/replica-sys/message.o \
./src/replica-sys/replica.o 

C_DEPS += \
./src/replica-sys/message.d \
./src/replica-sys/replica.d 


# Each subdirectory must supply rules for building sources it contributes
src/replica-sys/%.o: ../src/replica-sys/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc-4.8 -std=gnu11 -DDEBUG=$(DEBUGOPT) -I"$(ROOT_DIR)/../.local/include" -O0 -g3 -Wall -pg -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


