# Apple CoreAudio hooks 



Build julia hooks 
```
g++ -std=c++20 -O2 -dynamiclib \
  -Wl,-framework,CoreAudio \
  -Wl,-framework,AudioUnit \
  coreaudio.cpp julia_audio.cpp \
  -o libjulia_audio.dylib
```


[https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song.wav](A melody that solves the wave equation in julia)
