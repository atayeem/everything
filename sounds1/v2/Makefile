CXX=clang++
CXXFLAGS=-Wall -Wextra -g -O0

.PHONY: all clean

all: test test2 test_audio app

clean:
	rm -f test test_audio test2 test_gl_context out.wav out1.png

test: test.cpp libasi.cpp libasi.hpp
	$(CXX) libasi.cpp $< -lsndfile -lfftw3 -o $@

test2: test2.cpp libasi.cpp libasi.hpp
	$(CXX) libasi.cpp $< -lsndfile -lfftw3 -o $@

test_audio: test_audio.cpp
	$(CXX) $< $(shell pkg-config --cflags --libs sdl3) -lsndfile -o $@

app: app.cpp
	$(CXX) $< $(shell pkg-config --cflags --libs sdl3) -o $@

libwd: libwd.cpp
	$(CXX) $< $(shell pkg-config --cflags --libs sdl3) -o $@