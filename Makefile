ifeq ($(ERL_EI_INCLUDE_DIR),)
$(error ERL_EI_INCLUDE_DIR not set. Invoke via mix)
endif

# Set Erlang-specific compile and linker flags
ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR)

LDFLAGS ?= -fPIC -shared -pedantic
CFLAGS ?= -fPIC -O2

JOYSTICK_NIF=priv/joystick_nif.so

all: priv $(JOYSTICK_NIF)

priv:
	mkdir -p priv

$(JOYSTICK_NIF): c_src/joystick_nif.c
	$(CC) -o $@ $< $(LIBUSB_CFLAGS) $(ERL_CFLAGS) $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(LIBUSB_NIF)
	$(RM) $(JOYSTICK_NIF)
