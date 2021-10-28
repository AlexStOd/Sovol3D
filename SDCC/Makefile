# Toolchain definitions
CC = ~/Desktop/Sovol3D/sdcc-gas-master/bin/sdcc
LD = stm8-ld
AS = stm8-as

MKDIR = mkdir
CP = cp

DEFINE = -DSTM8S103

SPL_ROOT    = ../../..
SPL_SRC_DIR = $(SPL_ROOT)/Libraries/STM8S_StdPeriph_Driver/src
SPL_INC_DIR = $(SPL_ROOT)/Libraries/STM8S_StdPeriph_Driver/inc
SPL_SOURCE  = stm8s_gpio.c stm8s_i2c.c stm8s_clk.c stm8s_tim2.c
SPL_OBJECTS := $(addprefix $(OBJ_DIR)/, $(SPL_SOURCE:.c=.rel))


# Include settings
INCLUDE = $(addprefix -I, ../../../../../sdcc-gas-master/device/include/ $(SPL_INC_DIR)/ ../)
#~/Desktop/Sovol3D/sdcc-gas-master/device/include/
# Assembler flags
AS_FLAGS =

# Compiler flags
CC_FLAGS = -mstm8 --out-fmt-elf -c --debug --opt-code-size --asm=gas --function-sections --data-sections $(INCLUDE)

# Path definitions
PROJECT = STM8

# Objects definition
# Compiled objects list
OBJ_DIR = obj
VPATH += ../::$(SPL_SRC_DIR) ../../../../../sdcc-gas-master/device/lib
OBJECTS = $(addprefix $(OBJ_DIR)/, main.o aht20.o tm1621c.o keys.o stm8s_gpio.o stm8s_i2c.o stm8s_clk.o stm8s_tim2.o stm8s_it.o stm8s_adc1.o stm8s_flash.o _mullong.o _divulong.o _modsint.o _divsint.o _mulint.o _modulong.o _mulschar.o _divslong.o)

# Linker flags
LD_FLAGS = -Telf32stm8.x --print-memory-usage --gc-sections -Map $(OBJ_DIR)/map_$(PROJECT).map
LIB_DIRS = $(addprefix -L, ../../../../../sdcc-gas-master/device/lib/stm8 ../../../../../sdcc-gas-master/device/lib/stm8-large)
#LIB_DIRS = $(addprefix -L, /usr/local/share/sdcc/lib/stm8)

# Source dependencies:
DEPS = $(OBJECTS:.o=.d)
ASM_DEPS = $(OBJECTS:.o=.asm)

# ------------------------------------
# Instructions
# ------------------------------------

$(OBJ_DIR)/$(PROJECT).elf: $(OBJECTS)
	$(LD) $^ -o $@ $(LD_FLAGS) $(LIBS)

#-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)/

# Uncomment for standard generation

$(OBJ_DIR)/%.d: %.c
	@$(MKDIR) -p $(OBJ_DIR)
	$(CC) $< $(DEFINE) $(CC_FLAGS) -MM > $@

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d
	@$(MKDIR) -p $(OBJ_DIR)
	$(CC) $< $(DEFINE) $(CC_FLAGS) -o $@

$(OBJ_DIR)/%.o: %.asm
	@$(MKDIR) -p $(OBJ_DIR)
	$(AS) $< $(AS_FLAGS) -o $@

# ----------------------------------------
# Phony targets
# ----------------------------------------
.PHONY: clean debug