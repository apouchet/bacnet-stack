# Makefile to build LoRaWAN MQTT Decoder Service using GCC compiler
#
# This application is a production-grade LoRaWAN decoder service that:
# - Listens to LoRaWAN MQTT traffic
# - Dynamically loads JavaScript decoders based on device mapping
# - Decodes payloads using QuickJS JavaScript engine
# - Enriches frames and republishes them
#
# Dependencies:
#   - libmosquitto-dev (MQTT client library)
#   - libjson-c-dev (JSON parsing/serialization)
#   - libquickjs (JavaScript engine)
#
# Build with: make lorawan-decoder
# Or from this directory: make

# Executable file name
TARGET = lorawan-decoder

# Application source files
SRC = main.c \
	config.c \
	logging.c \
	sensor_map.c \
	decoder_cache.c \
	js_engine.c \
	mqtt_handler.c \
	frame_processor.c

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

OBJS += ${SRC:.c=.o}

# Additional libraries
MQTT_LIB = -lmosquitto
JSON_LIB = -ljson-c
JS_LIB = -L/usr/lib/quickjs -lquickjs -lm
PTHREAD_LIB = -lpthread

# Add extra libraries to linker flags
EXTRA_LFLAGS = $(MQTT_LIB) $(JSON_LIB) $(JS_LIB) $(PTHREAD_LIB)

# Use C99 for this application (needed for variable declarations after statements)
CSTANDARD = -std=c99

all: Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile
	${CC} ${PFLAGS} ${OBJS} ${EXTRA_LFLAGS} -Wl,-lc,-lgcc,-lrt,-lm -o $@
	size $@
	cp $@ ../../bin

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

.PHONY: depend
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

.PHONY: clean
clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map

.PHONY: include
include: .depend
