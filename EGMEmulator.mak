# Make command to use for dependencies
MAKE=make
RM=rm
MKDIR=mkdir

# Increment build number before building

# Set ZEUS_OS for Zeus platform builds
ZEUS_OS=1

# If no configuration is specified, "Release" will be used
ifndef CFG
CFG=Release
endif

#
# Configuration: Release
#
ifeq "$(CFG)" "Release"
OUTDIR=Release
OUTFILE=$(OUTDIR)/EGMEmulator
CFG_INC=-I. -Iinclude \
	-I/opt/fsl-imx-xwayland/5.4-zeus/sysroots/cortexa9t2hf-neon-poky-linux-gnueabi/usr/include/PCSC/ \
	-I/usr/include/PCSC \
	-I/opt/fsl-imx-xwayland/5.4-zeus/sysroots/cortexa9t2hf-neon-poky-linux-gnueabi/usr/include
CFG_LIB=-L/opt/fsl-imx-xwayland/5.4-zeus/sysroots/cortexa9t2hf-neon-poky-linux-gnueabi/usr/lib \
	-ls7lite -lpthread
CFG_OBJ=
COMMON_OBJ=$(OUTDIR)/EventService.o \
	$(OUTDIR)/Game.o \
	$(OUTDIR)/Machine.o \
	$(OUTDIR)/CommChannel.o \
	$(OUTDIR)/MachineCommPort.o \
	$(OUTDIR)/SASConstants.o \
	$(OUTDIR)/CRC16.o \
	$(OUTDIR)/BCD.o \
	$(OUTDIR)/SASCommands.o \
	$(OUTDIR)/SASCommPort.o \
	$(OUTDIR)/SASDaemon.o \
	$(OUTDIR)/MeterCommands.o \
	$(OUTDIR)/EnableCommands.o \
	$(OUTDIR)/ExceptionCommands.o \
	$(OUTDIR)/DateTimeCommands.o \
	$(OUTDIR)/TITOCommands.o \
	$(OUTDIR)/AFTCommands.o \
	$(OUTDIR)/ProgressiveCommands.o \
	$(OUTDIR)/main.o

# Platform-specific sources
ifdef ZEUS_OS
	COMMON_OBJ += $(OUTDIR)/ZeusSerialPort.o \
		$(OUTDIR)/ZeusPlatform.o
else
	COMMON_OBJ += $(OUTDIR)/SimulatedPlatform.o
endif

OBJ=$(COMMON_OBJ) $(CFG_OBJ)
ALL_OBJ=$(COMMON_OBJ) $(CFG_OBJ) \
	/opt/fsl-imx-xwayland/5.4-zeus/sysroots/cortexa9t2hf-neon-poky-linux-gnueabi/usr/lib/libs7lite.so.1.0.15 \
	-lpthread

COMPILE=$(CXX) -c -D_EGMEMULATOR -DZEUS_OS -g -Wall ${CXXFLAGS} -O2 -std=c++11 -lstdc++ -Wall -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wextra -Wno-sign-compare -Wno-type-limits -o "$(OUTDIR)/$(*F).o" $(CFG_INC) $<
LINK=$(CXX) -g -Wall -Wno-psabi ${CXXFLAGS} -O2 -std=c++11 -lstdc++ -rdynamic -ldl -pthread -o "$(OUTFILE)" $(ALL_OBJ)

# Pattern rules for event directory
$(OUTDIR)/%.o : src/event/%.cpp
	$(COMPILE)

# Pattern rules for simulator directory
$(OUTDIR)/%.o : src/simulator/%.cpp
	$(COMPILE)

# Pattern rules for io directory
$(OUTDIR)/%.o : src/io/%.cpp
	$(COMPILE)

# Pattern rules for sas directory
$(OUTDIR)/%.o : src/sas/%.cpp
	$(COMPILE)

# Pattern rules for sas/commands directory
$(OUTDIR)/%.o : src/sas/commands/%.cpp
	$(COMPILE)

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR) $(OBJ)
	$(LINK)

$(OUTDIR):
	$(MKDIR) -p "$(OUTDIR)"

# Target for building the sentinel.img file (named to match Zeus OS expectations)
# The EGMEmulator executable will be named "Sentinel" to match the OS startup script
sentinel: $(OUTFILE)
	@echo "Creating sentinel.img..."
	@$(RM) -rf sentinel_image
	@$(MKDIR) -p sentinel_image/opt/ncompass/bin
	@cp $(OUTFILE) sentinel_image/opt/ncompass/bin/Sentinel
	@chmod +x sentinel_image/opt/ncompass/bin/Sentinel
	@mksquashfs sentinel_image sentinel.img -noappend -comp gzip -all-root
	@$(RM) -rf sentinel_image
	@echo "sentinel.img created successfully!"
	@du -h sentinel.img

# Alias for compatibility
egmemulator: sentinel

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	$(RM) -f $(OUTFILE)
	$(RM) -f $(OBJ)
	$(RM) -f sentinel.img
	$(RM) -rf sentinel_image

# Clean this project and all dependencies
cleanall: clean
endif
