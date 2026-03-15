# Apple CoreAudio hooks 



Build CoreAudio hooks for julia 
```
g++ -std=c++20 -O2 -dynamiclib \
  -Wl,-framework,CoreAudio \
  -Wl,-framework,AudioUnit \
  coreaudio.cpp julia_audio.cpp \
  -o libjulia_audio.dylib
```

Samples: 

-  A melody that solves the wave equation in julia 
  - n = 24 ([mp3](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song.mp3) | [wav](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song.wav))
  - n = 128 ([mp3](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song_fine.mp3) | [wav](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song_fine.wav))


- Modal analysis (nearly sounds like a real string) 
  - n = 128 ([mp3](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song_ma.mp3) | [wav](https://github.com/josephmcl/coreaudio.hooks/raw/refs/heads/main/wave_song_ma.wav))
