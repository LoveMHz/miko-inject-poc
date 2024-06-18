XBE_TITLE = Fatal\ Frame\ -\ Injection\ POC
GEN_XISO = $(XBE_TITLE).iso
SRCS = $(CURDIR)/src/main.c $(CURDIR)/src/detour.c
CFLAGS="-D_XBOX"
CFLAGS += \
	-Os

BASE_GAME = $(CURDIR)/resources/default.xbe
TOOLS_DIR = $(CURDIR)/tools
MEDIA_DIR = $(OUTPUT_DIR)/media

include $(NXDK_DIR)/Makefile
LD = $(TOOLS_DIR)/nxdk-link-offset
CC = $(TOOLS_DIR)/nxdk-cc

$(GEN_XISO): $(MEDIA_DIR) $(BASE_GAME)

$(MEDIA_DIR):
	$(VE)cp -R $(patsubst $(OUTPUT_DIR)%,$(CURDIR)%,$@) '$@'

$(OUTPUT_DIR)/patch.xbe: main.exe $(OUTPUT_DIR) $(CXBE)
	@echo "[ CXBE     ] $@"
	$(VE)$(CXBE) -OUT:$@ -TITLE:$(XBE_TITLE) $< $(QUIET)

$(OUTPUT_DIR)/default.xbe: $(OUTPUT_DIR)/patch.xbe
	python3 $(CURDIR)/build_xbe.py
	#cp resources/default.xbe $(OUTPUT_DIR)/default.xbe
	mv out.xbe bin/default.xbe
