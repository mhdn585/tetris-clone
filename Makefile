# ============================================================================
#  Tetris de terminal - Makefile
#  Objetivos: all (juego), test (pruebas de logica), run, debug, clean
# ============================================================================

CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS := -Iinclude

TARGET   := tetris
BUILD    := build
SRCDIR   := src
TESTDIR  := tests

SRCS     := $(wildcard $(SRCDIR)/*.cpp)
OBJS     := $(patsubst $(SRCDIR)/%.cpp,$(BUILD)/%.o,$(SRCS))
HDRS     := $(wildcard include/*.hpp)

# Objetos de logica pura: se reutilizan en las pruebas unitarias.
CORE_OBJS := $(BUILD)/Tablero.o $(BUILD)/Tetramino.o $(BUILD)/Bolsa.o $(BUILD)/Puntuacion.o $(BUILD)/Datos.o
TEST_BIN  := $(BUILD)/tetris_tests

.PHONY: all run test debug clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRCDIR)/%.cpp $(HDRS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

# --- Pruebas de logica (sin terminal) ----------------------------------------
$(BUILD)/test_logica.o: $(TESTDIR)/test_logica.cpp $(HDRS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): $(CORE_OBJS) $(BUILD)/test_logica.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_BIN)
	./$(TEST_BIN)

# --- Varias -------------------------------------------------------------------
run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS := -std=c++20 -g -O0 -fsanitize=address,undefined -Wall -Wextra -Wpedantic
debug: clean $(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)
