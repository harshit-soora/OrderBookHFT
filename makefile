SRCS = test.cpp exchange.cpp orderManagement.cpp order.h orderManagement.h
OBJS = $(SRCS:.cpp=.o) 
MAIN = test

all: $(MAIN)

$(MAIN): $(OBJS) 
	g++ -Wall -g -o $(MAIN) $(OBJS)

clean:
	rm -rf $(MAIN) *.o
