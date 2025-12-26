CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include $(shell pkg-config --cflags libpipewire-0.3 fftw3f)
LDFLAGS = $(shell pkg-config --libs libpipewire-0.3 fftw3f) -lncurses -lpthread -lm

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = .

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/audiovis

# Default target
all: $(TARGET)

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Build executable
$(TARGET): $(OBJ_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Install (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/audiovis
	@echo "Installed to /usr/local/bin/audiovis"

# Uninstall
uninstall:
	rm -f /usr/local/bin/audiovis
	@echo "Uninstalled"

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install uninstall debug run
