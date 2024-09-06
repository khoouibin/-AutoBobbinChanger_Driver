#default optoin is build debug version
ver = debug
curr_dir = $(CURDIR)
obj_dir = $(curr_dir)/obj
output_dir = $(curr_dir)/output

ifeq ($(ver), release)
# build release version if the command is 'make ver=release'
build_version: echo_release build_release
CFLAGS = -c -O3 -D_ROLE_HOST
OBJPATH = ./obj/release
OUTPUT = output/release/rtc_driver	
else
# otherwise, build debug version.
build_version: echo_debug  mkdir_debug build_debug
CFLAGS = -c -g -D_DEBUG -D_ROLE_HOST
OBJPATH = ./obj/debug
OUTPUT = output/debug/abc_driver
endif

help:
	@echo "make ==> build debug version."
	@echo "make ver=debug ==> build debug version."
	@echo "make ver=release ==> build release version."
	@echo "make clean = delete all generated files."
	
echo_debug:
	@echo "Build debug version. Use \"make help\" to show all options."

echo_release:
	@echo "Build release version. Use \"make help\" to show all options."

mkdir_debug:
ifeq ("$(wildcard $(obj_dir))", "")
	@mkdir -p $(obj_dir)
else
	@rm -r $(obj_dir)
	@mkdir -p $(obj_dir)
endif
	@mkdir -p $(obj_dir)/debug

ifeq ("$(wildcard $(output_dir))", "")
	@mkdir -p $(output_dir)
else
	@rm -r $(output_dir)
	@mkdir -p $(output_dir)
endif
	@mkdir -p $(output_dir)/debug

CC=g++

LDFLAGS += -lpthread
LDFLAGS += -lusb-1.0
LDFLAGS += -lPocoUtil
LDFLAGS += -lPocoData
LDFLAGS += -lPocoFoundation
LDFLAGS += -lPocoNet
LDFLAGS += -lPocoJSON
LDFLAGS += -lthrift

SOURCE_PATH   = ./source
HEADER_FLAGS  = -I./header
HEADER_FLAGS += -I./header/COMMON
HEADER_FLAGS += -I./header/Debug
HEADER_FLAGS += -I../common/header

OBJS = $(OBJPATH)/main.o \
	$(OBJPATH)/notification_pkg.o \
	$(OBJPATH)/Mqtt_prot.o \
	$(OBJPATH)/logger_wrapper.o \
	$(OBJPATH)/Logger.o \
	$(OBJPATH)/cli.o \
	$(OBJPATH)/HandleCmd.o \
	$(OBJPATH)/interface_to_host.o \
	$(OBJPATH)/interface_driver_to_ui.o \
	$(OBJPATH)/httpinterface.o \
	$(OBJPATH)/rc5simple.o\
	$(OBJPATH)/iniparser.o \
	$(OBJPATH)/dictionary.o \
	$(OBJPATH)/BL_USBComm.o \
	$(OBJPATH)/bl_ctrl.o \
	$(OBJPATH)/update_sour.o \
	$(OBJPATH)/USBComm.o \
	$(OBJPATH)/USBComm_Driver_Linux64.o \
	$(OBJPATH)/Msg_Prot.o \

build_debug:$(OBJS)
	$(CC) -o $(OUTPUT) $(OBJS) $(LDFLAGS)

build_release:$(OBJS)
	$(CC) -o $(OUTPUT) $(OBJS) $(LDFLAGS)

$(OBJPATH)/main.o: $(SOURCE_PATH)/main.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/update_sour.o: $(SOURCE_PATH)/update_sour.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/Msg_Prot.o: $(SOURCE_PATH)/Msg_Prot.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/Mqtt_prot.o: $(SOURCE_PATH)/Mqtt_prot.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/interface_to_host.o: $(SOURCE_PATH)/interface_to_host.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/interface_driver_to_ui.o: $(SOURCE_PATH)/interface_driver_to_ui.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/notification_pkg.o: $(SOURCE_PATH)/notification_pkg.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/USBComm.o: $(SOURCE_PATH)/USBComm.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/USBComm_Driver_Linux64.o: $(SOURCE_PATH)/USBComm_Driver_Linux64.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

#$(OBJPATH)/RS485_comm.o: $(SOURCE_PATH)/RS485_comm.cpp
#	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/Logger.o: $(SOURCE_PATH)/Logger.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/cli.o: $(SOURCE_PATH)/cli.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/logger_wrapper.o: $(SOURCE_PATH)/logger_wrapper.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/HandleCmd.o: $(SOURCE_PATH)/HandleCmd.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@
	
$(OBJPATH)/httpinterface.o: $(SOURCE_PATH)/httpinterface.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/bl_ctrl.o: $(SOURCE_PATH)/bl_ctrl.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/rc5simple.o: $(SOURCE_PATH)/RC5Simple.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/iniparser.o: $(SOURCE_PATH)/iniparser.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/dictionary.o: $(SOURCE_PATH)/dictionary.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

$(OBJPATH)/BL_USBComm.o: $(SOURCE_PATH)/BL_USBComm.cpp
	$(CC) $(CFLAGS) $(HEADER_FLAGS) $^ -o $@

clean:
	@echo "Delete all output files. Use \"make help\" to show all options."
	@rm -f ./output/debug/abc_driver ./output/release/abc_driver ./obj/debug/*.o ./obj/release/*.o
	@rm -f -r $(obj_dir)
	@rm -f -r $(output_dir)