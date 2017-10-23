
INCS	=

DEFS  = -std=c++14 -O3 -lboost_program_options #-g

LIBS	=

LIBPATH =

COMP =	g++

all: mpcd

%.o: %.cpp %.hpp
	$(COMP) -c $< -o $@ $(INCS) $(DEFS) $(LIBPATH) $(LIBS)


mpcd: main.cpp random.o tools.o
	$(COMP) $^ -o $@ $(INCS) $(DEFS) $(LIBPATH) $(LIBS)

clean:
	rm -f *.o
