SEP = ;
EXEC = jsh

prog:
	gcc -Wall -c *c $(SEP) gcc -o $(EXEC) *.o -lreadline -lhistory

clean:
	rm *.o $(SEP) rm $(EXEC)