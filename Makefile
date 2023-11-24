SEP = ;
EXEC = jsh

prog:
	gcc -Wall -c *c $(SEP) gcc -o $(EXEC) *.o -lreadline

clean:
	rm *.o $(SEP) rm $(EXEC)