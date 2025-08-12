# my_byol
My Build Your Own Lisp experiment, following https://www.buildyourownlisp.com/

Run the unit tests:
```
lispy$ ut retest
lispy/_test.c ....                                 4 passed in 0.01s
lispy$ 
```

Run the interactive REPL: (integration test)
```
lispy$ ./run_lispy
cc   -lm  lispy.c   -o lispy
lispy> + 1 2 3
6
lispy>
```
