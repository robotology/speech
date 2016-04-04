Speech
=======
A simple text to speech using **`SVOXPICO`**.


Requirements
------------
The current implementation requires Linux **`aplay`** command to play the generated wave file.
This can/should be easily replaced with the `yarp portaudio` device (audio player)  


Installation
------------
$ cd speech
$ mkdir build && cd build
$ cmake ../ && make


Testing
-------
```sh
$ yarpdev --device speech --lingware-context svox-speech --default-language en-US --pitch 100 --speed 100
```

```sh
$ yarp rpc /icub/speech:rpc
>>help
Responses:
  *** Available commands:
  setLanguage
  setSpeed
  setPitch
  getSpeed
  getPitch
  getSupportedLang
  say
  help
```

```sh
>> say "Hello iCub!"
>> setPitch 200
>> say "Hello iCub!"
```
