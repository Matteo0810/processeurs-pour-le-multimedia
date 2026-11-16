CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
CPPFLAGS += -D_POSIX_C_SOURCE=200809L

SRC ?= implementations/code_sequentiel.c
TARGET = build/$(basename $(notdir $(SRC)))

ifeq ($(notdir $(SRC)),code_avx.c)
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
