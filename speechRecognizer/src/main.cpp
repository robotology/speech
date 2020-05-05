/*
* Copyright(C) 2014 WYSIWYD Consortium, European Commission FP7 Project ICT - 612139
* Authors: Stephane Lallee
* email : stephane.lallee@gmail.com
* Permission is granted to copy, distribute, and / or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* A copy of the license can be found at
* wysiwyd / license / gpl.txt
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General
* Public License for more details
*/

/**
\defgroup icub_speechRecognizer speechRecognizer

\section intro_sec Description
The module based on Microsoft Speech API. It is made to recognize speech based on runtime grammar in different formats:
 * XML W3C specification
 * "custom" simple format closed to the one used in the CSLU RAD Toolkit.
 The class RobotGrammarManager maintains a dictionary of various vocabulories usefull in HRI (Objects, Agents, Actions...) which can be
 expanded at runtime and can be used in grammars.

\section lib_sec Libraries
Microsoft speech sdk http://msdn.microsoft.com/en-us/library/hh361572(v=office.14).aspx

\section parameters_sec Parameters
--grammarFile specify the name of the grammar file to load at startup
--talkback if specified the program will use TTS to repeat every sentence recognized
 
\section portsa_sec Ports Accessed
If iSpeak is running then all text to speech commands will be forwarded to iSpeak.

\section portsc_sec Interface: Ports Created and data format
/modulename/recog/continuous:o broadcast commands recognized by the asynchronous grammar (see rpc commands "asynrecog" )
/moduleName/rpc handles rpc commands of type
 * "tts" "sentence to say"
    Say a sentence
 
 * "asyncrecog"
 
    ** "addGrammar"...
    Add a simple grammar (uses current state of RGM) to the asynchronous grammar
 
    ** "clear"...
    Reset the asynchrnous grammar to its original state
 
 * "recog"
    
    ** "timeout" "ms"
    Set the timeout for all recognition actions
 
    ** "choices" "choice1" "choice2"...
    Run a recognition over the list of choices passed in arguments. Returns the word/sentence recognized
    
    ** "dictation"
    Run a recognition of open dictation
 
    ** "grammarXML" "xml formatted grammar"
    Run the recognition of a grammar specified in the W3C SRGS xml format (http://www.w3.org/TR/speech-grammar/)
 
    ** "grammarSimple" "grammar to recognize"
    Run a recognition on a custom format using RobotGrammarManager, refer to RobotGrammarSpecification.txt

 * "RGM" "vocabulory"
    ** "get" "vocabuloryName"
     [NOT IMPLEMENTED YET] return a string containing all the vocabulory words
   
    ** "add" "vocabuloryName" "word"
    Expand a given vocabulory with a specific word
 
    ** "addAuto" "vocabuloryName"
    Expand a given vocabulory by putting the system in dictation mode and asking confirmation of what is being recognized

\section in_files_sec Input Data Files
None.

\section out_data_sec Output Data Files
None.
 
\section conf_file_sec Configuration Files
None.

\section tested_os_sec Tested OS
Windows 7. 

\author Stephane Lallee 
*/

#include "SpeechRecognizerModule.h"

int main(int argc, char* argv[])
{
    double t0;
    if (::CoInitializeEx(NULL,COINIT_MULTITHREADED) == S_OK)
    {
        Network yarp;
        if (!yarp.checkNetwork())
            return 1;

        ResourceFinder rf;
        rf.setDefaultContext("speechRecognizer");
        rf.setDefaultConfigFile("config.ini");
        rf.configure(argc,argv);
        SpeechRecognizerModule mod;

        mod.runModule(rf);
        t0 = Time::now();
        yInfo() <<"Out of the runModule(). You may want to stop the microphone to get back the hand...";
    }
    ::CoUninitialize();
    yInfo() <<"Time to give back the hand : "<<Time::now()-t0;
}


