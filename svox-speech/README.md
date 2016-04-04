Speech
=======
A simple text to speech using **`SVOXPICO`**.


Requirements
------------
The current implementation requires Linux **`aplay`** command (in the `alsa-utils` package)
to play the generated wave file. This can/should be easily replaced with the
`yarp portaudio` device (audio player)  


Installation
------------
- `$ cd svox-speech`
- `$ mkdir build && cd build`
- `$ cmake ..`
- tick on the driver `speech`.
- select a convenient location for installation via `CMAKE_INSTALL_PREFIX` (refer to this location as `$INST_DIR`).
- `$ make install`
- append to the environment variable **`YARP_DATA_DIRS`** the path `$INST_DIR/share/svox_speech`.


Testing
-------
```sh
$ yarpdev --device speech --lingware-context svox_speech --default-language en-US --pitch 100 --speed 100
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
