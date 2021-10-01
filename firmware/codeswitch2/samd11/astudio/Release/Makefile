################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
../include \
../include/component \
../include/instance \
../include/pio


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../../startup_samd11.c \
../../main.c \
../flash.c \
../keypad.c \
../lcd.c \
../rtc.c \
../tone.c \
../utils.c \
../wdt.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
startup_samd11.o \
main.o \
flash.o \
keypad.o \
lcd.o \
rtc.o \
tone.o \
utils.o \
wdt.o

OBJS_AS_ARGS +=  \
startup_samd11.o \
main.o \
flash.o \
keypad.o \
lcd.o \
rtc.o \
tone.o \
utils.o \
wdt.o

C_DEPS +=  \
startup_samd11.d \
main.d \
flash.d \
keypad.d \
lcd.d \
rtc.d \
tone.d \
utils.d \
wdt.d

C_DEPS_AS_ARGS +=  \
startup_samd11.d \
main.d \
flash.d \
keypad.d \
lcd.d \
rtc.d \
tone.d \
utils.d \
wdt.d

OUTPUT_FILE_PATH +=Demo.elf

OUTPUT_FILE_PATH_AS_ARGS +=Demo.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+= 


# AVR32/GNU C Compiler



















./%.o: ../../%.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.2.1
	$(QUOTE)D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -mthumb -D__SAMD11D14AM__ -D__SAMD11D14AM__ -DDONT_USE_CMSIS_INIT -DF_CPU=8000000 -DTESTING=1 -DKEYPAD_INTEGRATED=1 -DLCD=0  -I"../../include" -I"../.." -I".."  -Os -ffunction-sections -fdata-sections -Wall -mcpu=cortex-m0plus -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.2.1
	$(QUOTE)D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe$(QUOTE)  -x c -funsigned-char -funsigned-bitfields -mthumb -D__SAMD11D14AM__ -D__SAMD11D14AM__ -DDONT_USE_CMSIS_INIT -DF_CPU=8000000 -DTESTING=1 -DKEYPAD_INTEGRATED=1 -DLCD=0  -I"../../include" -I"../.." -I".."  -Os -ffunction-sections -fdata-sections -Wall -mcpu=cortex-m0plus -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker : 6.2.1
	$(QUOTE)D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -mthumb -Wl,--start-group  -Wl,--end-group -L"..\\Device_Startup"  -Wl,--gc-sections -mcpu=cortex-m0plus -Wl,--script=../../linker/samd11d14.ld --specs=nano.specs  
	@echo Finished building target: $@
	"D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -O binary "Demo.elf" "Demo.bin"
	"D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "Demo.elf" "Demo.hex"
	"D:\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-size.exe" "Demo.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "Demo.elf" "Demo.a" "Demo.hex" "Demo.bin" "Demo.lss" "Demo.eep" "Demo.map" "Demo.srec"
	