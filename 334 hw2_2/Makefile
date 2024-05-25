SRCS = simulator.c WriteOutput.c helper.c WriteOutput.h helper.h

make all: $(SRCS)
	gcc $(SRCS) -o simulator -lpthread

make try: $(SRCS)
	rm -f simulator
	gcc $(SRCS) -o simulator -lpthread
	./simulator < io/inputs/input0.txt