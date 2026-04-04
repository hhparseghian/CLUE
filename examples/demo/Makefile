CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -Wno-unused-parameter -O2
PKG     := clue

CFLAGS  += $(shell pkg-config --cflags $(PKG))
LDFLAGS += $(shell pkg-config --libs $(PKG))

SRCS := main.c page_widgets.c page_list.c page_form.c page_tree.c \
        page_table.c page_editor.c page_canvas.c page_3d.c page_splitter.c

OBJS := $(SRCS:.c=.o)
BIN  := clue_demo

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c demo.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean
