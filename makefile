
SRC_FILES=$(wildcard src/*.c src/**/*.c)
OBJ_FILES=$(subst .c,.o,$(SRC_FILES))

tractorbeam: CFLAGS += -W -Wall -O2
tractorbeam: override CFLAGS += -Isrc -std=c99 -pedantic
tractorbeam: $(OBJ_FILES)
	$(CC) -o $(OBJ_FILES) -o $@ $< -lzookeeper_mt

clean:
	rm -f $(OBJ_FILES)
	rm -f tractorbeam
