# Compiler settings
CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Werror -O2 -g3
LDFLAGS := -lkrpc -lprotobuf -lz

# Project files
SOURCES := SubOrbitalFlight.cpp main.cpp debug.cpp
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLE := SubOrbitalFlightExecutable

# Default rule
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: clean
