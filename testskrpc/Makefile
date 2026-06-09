# # Compiler settings
# CXX := g++
# CXXFLAGS := -std=c++11 -Wall -Wextra -O2 -I/media/dev/6_To_Backup2/Dev/krpc-cpp-0.5.4/include -I/media/dev/6_To_Backup2/Dev/krpc-cpp-0.5.4/src
# # CXXFLAGS := -std=c++11 -Wall -Wextra -O2
# LDFLAGS := -lkrpc -lprotobuf -lz

# # Project files
# SOURCES := SubOrbitalFlight.cpp main.cpp
# OBJECTS := $(SOURCES:.cpp=.o)
# EXECUTABLE := SubOrbitalFlightExecutable

# # Default rule
# all: $(EXECUTABLE)

# $(EXECUTABLE): $(OBJECTS)
# 	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# %.o: %.cpp
# 	$(CXX) -c $< -o $@ $(CXXFLAGS)

# clean:
# 	rm -f $(OBJECTS) $(EXECUTABLE)

# .PHONY: clean
# Compiler settings
CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -O2 -I/media/dev/6_To_Backup2/Dev/krpc-cpp-0.5.4/include

# Add the local directory to the library path so it can find libkrpc-0.5.4.so
LDFLAGS := -L/media/dev/6_To_Backup2/Dev/krpc-cpp-0.5.4/src/.libs -Wl,-rpath,/media/dev/6_To_Backup2/Dev/krpc-cpp-0.5.4/src/.libs -lkrpc -lprotobuf -lz

# Project files (Now including your freshly generated proto file)
SOURCES := SubOrbitalFlight.cpp main.cpp krpc.pb.cpp
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLE := SubOrbitalFlightExecutable

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: clean