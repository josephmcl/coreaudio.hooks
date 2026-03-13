all: shout

shout: *.h *.cpp
	g++ -std=c++2a -pthread -Wl,-framework,CoreAudio -Wl,-framework,AudioUnit coreaudio.cpp main.cpp -o shout

clean:
	rm -rf shout

PHONY: clean
