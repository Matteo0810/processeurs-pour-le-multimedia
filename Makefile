CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11

SRC ?= implementations/code_sequentiel.c
TARGET = build/code_sequentiel

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	@if [ -z "$(IMAGE)" ]; then \
		echo "Usage : make run IMAGE=img/image1.pgm"; \
		exit 1; \
	fi
	./$(TARGET) $(IMAGE)

clean:
	rm -rf build *_new.pgm