################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/prjct3.c \
../src/syscalls.c \
../src/system_stm32f0xx.c 

OBJS += \
./src/prjct3.o \
./src/syscalls.o \
./src/system_stm32f0xx.o 

C_DEPS += \
./src/prjct3.d \
./src/syscalls.d \
./src/system_stm32f0xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F051R8Tx -DSTM32F0DISCOVERY -DDEBUG -DSTM32F051 -DUSE_STDPERIPH_DRIVER -I"/Users/Krister/Documents/ECE362_workspace/project/Utilities" -I"/Users/Krister/Documents/ECE362_workspace/project/StdPeriph_Driver/inc" -I"/Users/Krister/Documents/ECE362_workspace/project/inc" -I"/Users/Krister/Documents/ECE362_workspace/project/CMSIS/device" -I"/Users/Krister/Documents/ECE362_workspace/project/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


