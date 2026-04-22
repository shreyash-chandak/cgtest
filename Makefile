# ──────────────────────────────────────────────────────────────
#  Makefile — Hierarchical Scene Modelling (CG Assignment 3)
#  Build: make          Run: make run         Clean: make clean
# ──────────────────────────────────────────────────────────────

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Isrc
LDFLAGS  = -lGL -lGLEW -lglfw -lm -ldl
TARGET   = scene

SRCS = main.cpp     \
       globals.cpp  \
       shader.cpp   \
       texture.cpp  \
       mesh.cpp     \
       world.cpp    \
       input.cpp    \
       camera.cpp   \
       update.cpp   \
       render.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run:
	make all && ./$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean run
