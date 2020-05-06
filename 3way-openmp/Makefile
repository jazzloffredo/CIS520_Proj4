IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj

_DEPS = queue.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = scorecard_openmp.o queue.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c $(DEPS)
	if [ ! -d "obj" ]; then mkdir obj; fi
	$(CC) -fopenmp -lpthread -lrt -std=c99 -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -fopenmp -lpthread -lrt -std=c99 -o openmp $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -rf $(ODIR) openmp