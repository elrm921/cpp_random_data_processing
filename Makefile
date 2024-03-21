NAME = MyTestClass
WORKDIR := $(shell pwd)

_default: 
	$(MAKE) compile

clean:
	rm -f $(NAME).o

compile: 
	g++ -g -std=c++17 main.cpp classes.cpp -o $(NAME).o
