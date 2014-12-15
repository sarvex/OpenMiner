#---------------------------------------------------------------------------------
# Executable name
#---------------------------------------------------------------------------------
TARGET		:=  $(shell basename $(CURDIR))

#---------------------------------------------------------------------------------
# Compiler executables
#---------------------------------------------------------------------------------
CC		:=	gcc
CXX		:=	g++

#---------------------------------------------------------------------------------
# Options for code generation
#---------------------------------------------------------------------------------
CFLAGS	:=	-g -Wall
CXXFLAGS:=	$(CFLAGS) -std=c++11 -MD
LDFLAGS	:=	-g

#---------------------------------------------------------------------------------
# Any extra libraries you wish to link with your project
#---------------------------------------------------------------------------------
LIBS	:= -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

#---------------------------------------------------------------------------------
ifeq ($(shell uname), Darwin)
#---------------------------------------------------------------------------------
LIBS	+=	-framework OpenGL
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
LIBS	+=	-lGLEW -lGL
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Source folders
#---------------------------------------------------------------------------------
BUILD		:=	build
SOURCES		:=	source external $(wildcard source/*)
INCLUDES	:=	include external $(wildcard include/*)

#---------------------------------------------------------------------------------
# Additional folders for libraries
#---------------------------------------------------------------------------------
LIBDIRS		:= 	

#---------------------------------------------------------------------------------
# Source files
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export CFILES	:=	$(sort $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c))))
export CPPFILES	:=	$(sort $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp))))

SOURCEFILES	:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)) \
				$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
HEADERFILES	:=	$(foreach dir,$(INCLUDES),$(wildcard $(dir)/*.h)) \
				$(foreach dir,$(INCLUDES),$(wildcard $(dir)/*.hpp))

#---------------------------------------------------------------------------------
# Use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir))

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(CURDIR)/$(dir))

#---------------------------------------------------------------------------------
.PHONY: $(BUILD) clean run install uninstall
#------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
run:
	@echo running ...
	@./$(TARGET)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET)
	
#---------------------------------------------------------------------------------
install:
	@cp -u $(TARGET) /usr/local/bin/$(TARGET)
	@echo installed.

#---------------------------------------------------------------------------------
uninstall:
	@rm -f /usr/local/bin/$(TARGET)
	@echo uninstalled.

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
# Makefile targets
#---------------------------------------------------------------------------------
all: $(OUTPUT)
	
#---------------------------------------------------------------------------------
$(OUTPUT): $(OFILES)
	@echo built ... $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

#---------------------------------------------------------------------------------
%.o: %.c
	@echo $(notdir $<)
	@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
	
#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@
	
-include *.d

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

