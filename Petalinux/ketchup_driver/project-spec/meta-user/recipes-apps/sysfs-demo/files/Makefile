APP = sysfs-demo

# Add any other object files to this list below
APP_OBJS = sysfs-demo.o ketchup_lib/ketchup_lib_common.o ketchup_lib/ketchup_lib_hardware.o


all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(LDFLAGS) $(LDLIBS)
clean:
	rm -f $(APP) *.o

