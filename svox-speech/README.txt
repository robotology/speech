Speech
=======
A simple text to speech using SVOXPICO! 


Requirements
------------
The current implemnetaion requires Linux 'aplay' command to play the generated wave file. 
This can/should be easily replaced with the yarp portaudio devide (audio player)  


Installation
------------
$ cd speech 
$ mkdir build; cd build
$ cmake ../; make 


Testing 
-------
$ yarpdev --device speech --lingware-path <path to the tts-lang dir> --pitch 100 --speed 100

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

>> say "Hello iCub!"
>> setPitch 200
>> say "Hello iCub!"




