all: scheduler.c
	gcc scheduler.c process.c -o scheduler

clean:
	rm -f scheduler