TARGET:=lispy
ifdef D
RUN:=gdb -q -ex r --args
else
RUN:=valgrind -q --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --exit-on-first-error=yes --error-exitcode=128
endif
all: lispy
	$(RUN) ./lispy
lispy: CFLAGS=-O0 -g
lispy: LDFLAGS=-lm
clean:
	$(RM) $(TARGET)
