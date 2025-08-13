TARGET:=lispy
ifdef D
GDB:=gdb -q -ex r --args
else
GDB:=valgrind -q --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --exit-on-first-error=yes --error-exitcode=128
endif
all: lispy
	$(GDB) ./lispy
lispy: CFLAGS=-O0 -g
lispy: LDFLAGS=-lm
clean:
	$(RM) $(TARGET)
