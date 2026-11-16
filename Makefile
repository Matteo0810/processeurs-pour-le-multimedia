CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
CPPFLAGS += -D_POSIX_C_SOURCE=200809L

SRC ?= implementations/code_sequentiel.c
TARGET = build/$(basename $(notdir $(SRC)))

ifneq ($(filter code_avx_short.c code_avx_char.c code_avx_int.c,$(notdir $(SRC))),)
SIMD_FLAGS = -mavx
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SIMD_FLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	@if [ -z "$(IMAGE)" ]; then \
		echo "Usage : make run IMAGE=img/image1.pgm"; \
		exit 1; \
	fi
	./$(TARGET) $(IMAGE)

clean:
	rm -rf build *_new.pgm
