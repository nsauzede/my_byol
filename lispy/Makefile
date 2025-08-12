TARGET:=lispy
ifdef D
GDB:=gdb -q -ex r --args
endif
all: lispy
	$(GDB) ./lispy
lispy: CFLAGS=-O0 -g
lispy: LDFLAGS=-lm
clean:
	$(RM) $(TARGET)
