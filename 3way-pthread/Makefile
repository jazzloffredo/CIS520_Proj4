IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj

_DEPS = queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = scorecard_pthread.o queue.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c $(DEPS)
	if [ ! -d "obj" ]; then mkdir obj; fi
	$(CC) -lpthread -lrt -std=c99 -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -lpthread -lrt -std=c99 -o pthread $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -rf $(ODIR) pthread