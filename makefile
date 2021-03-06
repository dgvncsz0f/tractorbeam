
bin_ronn ?= ronn

SRC_FILES=$(wildcard src/*.c src/**/*.c)
OBJ_FILES=$(subst .c,.o,$(SRC_FILES))

TRACTORBEAM=tractorbeam

$(TRACTORBEAM): CFLAGS += -W -Wall -O2
$(TRACTORBEAM): override CFLAGS += -Isrc -std=c99 -pedantic
$(TRACTORBEAM): $(OBJ_FILES)
	$(CC) -o $(OBJ_FILES) -o $@ $< -lzookeeper_mt

manpages:
	$(bin_ronn) -r man/tractorbeam.ronn

clean:
	rm -f $(OBJ_FILES)
	rm -f $(TRACTORBEAM)
	rm -f man/tractorbeam.1
