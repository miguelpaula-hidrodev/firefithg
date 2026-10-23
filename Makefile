#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set the DEVKITARM environment variable in your environment.")
endif

include $(DEVKITARM)/ds_rules

# Nome do arquivo final do seu jogo
TARGET		:=	WW3DoomDS
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include

# Configurações do processador ARM do Nintendo DS (Adicionado as specs do ARM9 no topo)
ARCH	:=	-mthumb -mthumb-interwork -specs=ds_arm9.specs

CFLAGS	:=	-g -Wall -O2\
			-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
			-ffast-math \
			-DARM9 \
			$(ARCH)

CXXFLAGS:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -mno-fpu

# Biblioteca oficial do Nintendo DS
LIBS	:=	-lnds9

# Regras automáticas de compilação
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export OFILES	:=	$(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(SOURCES),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD) \
					-I$(DEVKITPRO)/libnds/include

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $@ -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds

else

$(OUTPUT).nds: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

%.o: %.cpp
	@echo $(notdir $<)
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(INCLUDE) -c $< -o $@

-include $(OFILES:.o=.d)

endif
