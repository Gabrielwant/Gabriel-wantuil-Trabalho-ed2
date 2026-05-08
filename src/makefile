CC     = gcc
CFLAGS = -std=c99 -fstack-protector-all -Wall -Wextra -g
LFLAGS = -lm

SRC    = src
BUILD  = build
BIN    = bin

SRCS   = $(SRC)/main.c $(SRC)/geo.c $(SRC)/pm.c $(SRC)/qry.c $(SRC)/svg.c $(SRC)/hashfile.c
OBJS   = $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

TARGET = $(BIN)/ted.exe

UNITY  = $(SRC)/unity.c

TEST_HF  = $(BIN)/test_hashfile.exe
TEST_GEO = $(BIN)/test_geo.exe
TEST_PM  = $(BIN)/test_pm.exe

.PHONY: all clean tests run_tests dirs

# -------- padrão --------
all: dirs $(TARGET)

# -------- cria pastas --------
dirs:
	if not exist $(BUILD) mkdir $(BUILD)
	if not exist $(BIN) mkdir $(BIN)

# -------- executável principal --------
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# -------- objetos --------
$(BUILD)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# -------- testes --------
$(TEST_HF): $(SRC)/test_hashfile.c $(SRC)/hashfile.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TEST_GEO): $(SRC)/test_geo.c $(SRC)/geo.c $(SRC)/hashfile.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TEST_PM): $(SRC)/test_pm.c $(SRC)/pm.c $(SRC)/geo.c $(SRC)/hashfile.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

tests: dirs $(TEST_HF) $(TEST_GEO) $(TEST_PM)

# -------- rodar testes --------
run_tests: tests
	@echo === test_hashfile ===
	$(TEST_HF)
	@echo === test_geo ===
	$(TEST_GEO)
	@echo === test_pm ===
	$(TEST_PM)

# -------- limpeza --------
clean:
	del /Q $(BUILD)\*.o 2>nul
	del /Q $(BIN)\*.exe 2>nul