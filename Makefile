SEP = ;
EXEC = jsh

prog:
	gcc -Wall -c *c $(SEP) gcc -o $(EXEC) *.o -lreadline -lhistory

progMac:

	gcc -Wall -c *c $(SEP) gcc -o $(EXEC) *.o -L/usr/local/opt/readline/lib -lreadline -lhistory

clean:
	rm *.o $(SEP) rm $(EXEC)