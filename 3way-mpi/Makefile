IDIR =./include
CC=mpicc

ODIR=obj

_OBJ = scorecard_mpi.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c
	if [ ! -d "obj" ]; then mkdir obj; fi
	$(CC) -std=c99 -c -o $@ $<

all: $(OBJ)
	$(CC) -std=c99 -o mpi $^

.PHONY: clean

clean:
	rm -rf $(ODIR) mpi