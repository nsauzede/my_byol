# UT project: "my_byol", from UT version 0.0.14
# WARNING: do not put any explicit Make targets in this file!

UT_FAST:=1
UT_SLOW:=0
UT_NOPY:=1
UT_NOGT:=1
UT_VERBOSE:=0
UT_NOCAP:=1

# Usual macros (CFLAGS, CXXFLAGS, LDFLAGS, LDLIBS, LD_LIBRARY_PATH, ..) can be defined, eg:
#CXXFLAGS:=-I this/path -D THAT_SYMBOL ...
# Or even UT internal ones, like VGO (valgrind options), eg:
#VGO:=--suppressions=my_vg.supp --gen-suppressions=all
#UT_CUSTOM_ALL+=<custom 'all' additional targets>
#UT_CUSTOM_DEPS+=<custom additional dependencies>
CFLAGS:=-std=c99 -D_POSIX_C_SOURCE=199309L
LDFLAGS:=-lm
