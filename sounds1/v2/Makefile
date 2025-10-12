CXX=clang++
CXXFLAGS=-Wall -Wextra -g -O0

.PHONY: all

all: test test2 test_audio

test: test.cpp libasi.cpp libasi.hpp
	$(CXX) libasi.cpp $< -lsndfile -lfftw3 -o $@

test2: test2.cpp libasi.cpp libasi.hpp
	$(CXX) libasi.cpp $< -lsndfile -lfftw3 -o $@

test_audio: test_audio.cpp
	$(CXX) $< $(shell pkg-config --cflags --libs sdl3) -lsndfile -o $@

test_gl_context: test_gl_context.cpp
	$(CXX) $< ../glad/src/glad.c $(shell pkg-config --cflags --libs sdl3) -lm -lglfw -lGL -L/usr/local/lib -I ../glad/include