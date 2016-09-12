UNAME=$(shell uname)
CC=gcc
ifeq ($(UNAME),Darwin)
OBJ=sexp.o fmemopen/fmemopen.o
FLAGS=-Wno-tautological-compare
else
OBJ=sexp.o
FLAGS=
endif

sexp: $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $< $(FLAGS)

clean:
	rm -f sexp $(OBJ)
