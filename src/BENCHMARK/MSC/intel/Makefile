include ../../../Makefile.inc

SRC=copy.c mov.c flops.c MSC.c validation.c
OBJ=$(patsubst %.c, %.o, $(SRC))
CFLAGS+=-march=native
#CFLAGS+=-D__FMA__ -D __AVX512__

all: MSC.a

validation.o: validation.c
	$(CC) $(CFLAGS) -DINCLUDE_DIR=$(HEADER_DIR) -DBENCH_DIR=$(PWD)/.. -DLIB_DIR=$(LIB_DIR) -DCC=$(CC) -DOMP_FLAG=$(OMP_FLAG) -fPIC -Wl,-rpath=$(LIB_DIR) -Wl,-rpath==$(PWD)/.. -c -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $^

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $^

MSC.a: $(OBJ)
	ar rcs $@ $(OBJ)

clean:
	rm -rf $(OBJ) *.s MSC.a

