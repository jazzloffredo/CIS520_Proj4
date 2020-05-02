CC=gcc

%.o: %.c
	$(CC) -std=c99 -c -o $@ $<

.PHONY: all linear batch clean

all: linear batch

submit: all
	number=1 ; while [[ $$number -le 10 ]] ; do \
		sbatch ./scripts/submit_batch.sh ; \
		sbatch ./scripts/submit_linear.sh ; \
		((number = number + 1)) ; \
	done

linear:	mkexecdir src/scorecard_serial_linear.o
	$(CC) -std=c99 -o execs/linear src/scorecard_serial_linear.o

batch: mkexecdir src/scorecard_serial_batch.o 
	$(CC) -std=c99 -o execs/batch src/scorecard_serial_batch.o

src = $(wildcard src/*.c)
obj = $(src:.c=.o)

mkexecdir:
	if [ ! -d "./execs" ]; then mkdir execs; fi

clean:
	rm -rf $(obj) execs