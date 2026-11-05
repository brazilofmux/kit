CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I../../librs
LDFLAGS = -L../../librs -lrs -lqrencode -lzbar -lz

TARGETS = qrtapemake qrtaperead

all: $(TARGETS)

qrtapemake: main.o Encoder.o
	$(CXX) $^ -o $@ $(LDFLAGS)

qrtaperead: read_main.o Decoder.o
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean

# Dependencies
main.o: Encoder.h qrtape_defs.h
Encoder.o: Encoder.h qrtape_defs.h
read_main.o: Decoder.h qrtape_defs.h
Decoder.o: Decoder.h qrtape_defs.h