/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini
 * email:  ugo.pattacini@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

/**
\defgroup iSpeak iSpeak

Acquire sentences over a yarp port and then let the robot
utter them, also controlling the facial expressions.

\section intro_sec Description

The behavior is pretty intuitive and does not need any further
detail.\n

\section lib_sec Libraries
- YARP libraries.
- Packages for speech synthesis (e.g. speech-dev, festival, espeak, ...).

\section parameters_sec Parameters
--name \e name
- The parameter \e name identifies the unique stem-name used to
  open all relevant ports.

--period \e T
- The period given in [ms] for controlling the mouth.

--package \e pck
- The parameter \e pck specifies the package used for utterance;
  e.g. "speech-dev", "festival", "acapela-tts", "espeak", ...\n
  If \e speech-dev is specified then the yarp speech device is employed.

--package_options \e opt
- The parameter \e opt is a string specifying further
  command-line options to be used with the chosen package. Refer
  to the package documentation for the available options. In case
  the \e speech-dev package is used, this option should contain
  property-like rpc commands.

\section portsc_sec Ports Created
- \e /<name>: this port receives the string for speech
  synthesis. In case a double is received in place of a string,
  then the mouth will be controlled without actually uttering
  any word; that double accounts for the uttering time. \n
  Optionally, as second parameter available in both modalities,
  an integer can be provided that overrides the default period
  used to control the mouth, expressed in [ms]. Negative values
  are not processed and serve as placeholders. \n Finally,
  available only in string mode, a third double can be provided
  that establishes the uttering duration in seconds,
  irrespective of the words actually spoken.

- \e /<name>/emotions:o: this port serves to command the facial
  expressions of the iCub.
  
- \e /<name>/r1:rpc: this port serves to command the R1's mouth.
   
- \e /<name>/rpc: a remote procedure call port used for the
  following run-time querie: \n
  - [stat]: returns "speaking" or "quiet".
  - [set] [opt] "package_options": set new package dependent
   command-line options.
  - [get] [opt]: returns a string containing the current
   package dependent command-line options.

- \e /<name>/speech-dev/rpc: this port serves to talk to speech yarp device.

\section tested_os_sec Tested OS
Linux and Windows.

\author Ugo Pattacini
*/

#include <cstdlib>
#include <mutex>
#include <string>
#include <deque>

#include <yarp/os/all.h>

using namespace std;
using namespace yarp::os;


/************************************************************************/
class MouthHandler : public PeriodicThread
{
    string state;
    RpcClient emotions,r1;
    mutex mtx;
    double t0, duration;

    /************************************************************************/
    void send()
    {
        if (emotions.getOutputCount()>0)
        {
            Bottle cmd, reply;
            cmd.addVocab32("set");
            cmd.addVocab32("mou");
            cmd.addVocab32(state);
            emotions.write(cmd,reply);
        }
    }

    /************************************************************************/
    void run()
    {
        mtx.lock();

        if (state=="sur")
            state="hap";
        else
            state="sur";

        send();

        mtx.unlock();

        if (duration>=0.0)
            if (Time::now()-t0>=duration)
                suspend();
    }

    /************************************************************************/
    bool threadInit()
    {
        if (r1.getOutputCount()>0)
        {
            Bottle cmd,rep;
            cmd.addVocab32("tstart");
            r1.write(cmd,rep);
        }

        t0=Time::now();
        return true;
    }

    /************************************************************************/
    void threadRelease()
    {
        emotions.interrupt();
        r1.interrupt();
        emotions.close();
        r1.close();
    }

public:
    /************************************************************************/
    MouthHandler() : PeriodicThread(1.0), duration(-1.0) { }

    /************************************************************************/
    void configure(ResourceFinder &rf)
    {
        string name=rf.find("name").asString();
        emotions.open("/"+name+"/emotions:o");
        r1.open("/"+name+"/r1:rpc");

        state="sur";
        setPeriod((double)rf.check("period",Value(200)).asInt32()/1000.0);
    }

    /************************************************************************/
    void setAutoSuspend(const double duration)
    {
        this->duration=duration;
    }

    /************************************************************************/
    void resume()
    {
        if (r1.getOutputCount()>0)
        {
            Bottle cmd,rep;
            cmd.addVocab32("tstart");
            r1.write(cmd,rep);
        }

        t0=Time::now();
        PeriodicThread::resume();
    }

    /************************************************************************/
    void suspend()
    {
        if (isSuspended())
            return;

        PeriodicThread::suspend();

        lock_guard<mutex> lg(mtx);
        state="hap";
        send();

        if (r1.getOutputCount()>0)
        {
            Bottle cmd,rep;
            cmd.addVocab32("tstop");
            r1.write(cmd,rep);
        }
    }
};


/************************************************************************/
class iSpeak : protected BufferedPort<Bottle>,
               public    PeriodicThread,
               public    PortReport
{
    string name;
    string package;
    string package_options;
    deque<Bottle> buffer;
    mutex mtx;

    bool speaking;
    MouthHandler mouth;
    RpcClient speechdev;
    int initSpeechDev;

    /************************************************************************/
    void report(const PortInfo &info)
    {
        if (info.created && !info.incoming)
            initSpeechDev++;
    }

    /************************************************************************/
    void onRead(Bottle &request)
    {
        lock_guard<mutex> lg(mtx);
        buffer.push_back(request);
        speaking=true;
    }

    /************************************************************************/
    bool threadInit()
    {
        open("/"+name);
        useCallback();
        return true;
    }

    /************************************************************************/
    void threadRelease()
    {
        mouth.stop();
        if (speechdev.asPort().isOpen())
            speechdev.close();
        interrupt();
        close();
    }

    /************************************************************************/
    void execSpeechDevOptions()
    {
        lock_guard<mutex> lg(mtx);
        if (speechdev.getOutputCount()>0)
        {
            Bottle options(package_options);
            for (int i=0; i<options.size(); i++)
            {
                if (Bottle *opt=options.get(i).asList())
                {
                    Bottle cmd,rep;
                    cmd=*opt;
                    yInfo("Setting option: %s",cmd.toString().c_str());
                    speechdev.write(cmd,rep);
                    yInfo("Received reply: %s",rep.toString().c_str());
                }
            }
        }
    }

    /************************************************************************/
    void speak(const string &phrase)
    {
        lock_guard<mutex> lg(mtx);
        if (speechdev.asPort().isOpen())
        {
            if (speechdev.getOutputCount()>0)
            {
                Bottle cmd,rep;
                cmd.addString("say");
                cmd.addString(phrase);
                speechdev.write(cmd,rep);
            }
        }
        else
        {
            string command("echo \"");
            command+=phrase;
            command+="\" | ";
            command+=package;
            command+=" ";

            if (package=="festival")
                command+="--tts ";

            command+=package_options;
            int ret=system(command.c_str());
        }
    }

    /************************************************************************/
    void run()
    {
        string phrase;
        double time;
        bool onlyMouth=false;
        int rate=(int)(1000.0*mouth.getPeriod());
        bool resetRate=false;
        double duration=-1.0;

        mtx.lock();
        if (buffer.size()>0)    // protect also the access to the size() method
        {
            Bottle request=buffer.front();
            buffer.pop_front();

            if (request.size()>0)
            {
                if (request.get(0).isString())
                    phrase=request.get(0).asString();
                else if (request.get(0).isFloat64() || request.get(0).isInt32())
                {
                    time=request.get(0).asFloat64();
                    onlyMouth=true;
                }

                if (request.size()>1)
                {
                    if (request.get(1).isInt32())
                    {
                        int newRate=request.get(1).asInt32();
                        if (newRate>0)
                        {
                            mouth.setPeriod((double)newRate/1000.0);
                            resetRate=true;
                        }
                    }

                    if ((request.size()>2) && request.get(0).isString())
                        if (request.get(2).isFloat64() || request.get(2).isInt32())
                            duration=request.get(2).asFloat64();
                }
            }
        }
        mtx.unlock();

        if (speaking)
        {
            mouth.setAutoSuspend(duration);
            if (mouth.isSuspended())
                mouth.resume();
            else
                mouth.start();

            if (onlyMouth)
                Time::delay(time);
            else
                speak(phrase);

            mouth.suspend();
            if (resetRate)
                mouth.setPeriod((double)rate/1000.0);

            lock_guard<mutex> lg(mtx);
            if (buffer.size()==0)
                speaking=false;
        }

        if (initSpeechDev>0)
        {
            yInfo("Setting options at connection time");
            execSpeechDevOptions();
            initSpeechDev=0;
        }
    }

public:
    /************************************************************************/
    iSpeak() : PeriodicThread(0.2)
    {
        speaking=false;
        initSpeechDev=0;
    }

    /************************************************************************/
    void configure(ResourceFinder &rf)
    {
        name=rf.find("name").asString();
        package=rf.find("package").asString();
        package_options=rf.find("package_options").asString();

        mouth.configure(rf);

        if (package=="speech-dev")
        {
            speechdev.open("/"+name+"/speech-dev/rpc");
            speechdev.setReporter(*this);
        }

        yInfo("iSpeak wraps around \"%s\" speech synthesizer",package.c_str());
        yInfo("starting command-line options: \"%s\"",package_options.c_str());
    }

    /************************************************************************/
    bool isSpeaking()
    {
        lock_guard<mutex> lg(mtx);
        return speaking;
    }

    /************************************************************************/
    string get_package_options() const
    {
        return package_options;
    }

    /************************************************************************/
    void set_package_options(const string &package_options)
    {
        this->package_options=package_options;
        if (speechdev.asPort().isOpen())
            execSpeechDevOptions();
    }
};


/************************************************************************/
class Launcher: public RFModule
{
protected:
    iSpeak speaker;
    RpcServer rpc;

public:
    /************************************************************************/
    bool configure(ResourceFinder &rf)
    {
        speaker.configure(rf);
        if (!speaker.start())
            return false;

        string name=rf.find("name").asString();
        rpc.open("/"+name+"/rpc");
        attach(rpc);

        return true;
    }

    /************************************************************************/
    bool close()
    {
        rpc.close();
        speaker.stop();
        return true;
    }

    /************************************************************************/
    bool respond(const Bottle &command, Bottle &reply)
    {
        int cmd0=command.get(0).asVocab32();
        if (cmd0==Vocab32::encode("stat"))
        {
            reply.addString(speaker.isSpeaking()?"speaking":"quiet");
            return true;
        }

        if (command.size()>1)
        {
            int cmd1=command.get(1).asVocab32();
            if (cmd1==Vocab32::encode("opt"))
            {
                if (cmd0==Vocab32::encode("set"))
                {
                    if (command.size()>2)
                    {
                        string cmd2=command.get(2).asString();
                        speaker.set_package_options(cmd2);
                        reply.addString("ack");
                        return true;
                    }
                }
                else if (cmd0==Vocab32::encode("get"))
                {
                    reply.addString(speaker.get_package_options());
                    return true;
                }
            }
        }

        return RFModule::respond(command,reply);
    }

    /************************************************************************/
    double getPeriod()
    {
        return 1.0;
    }

    /************************************************************************/
    bool updateModule()
    {
        return true;
    }
};


/************************************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("YARP server not available!");
        return 1;
    }

    ResourceFinder rf;
    rf.setDefault("name","iSpeak");
    rf.setDefault("package","speech-dev");
    rf.setDefault("package_options","");
    rf.configure(argc,argv);

    Launcher launcher;
    return launcher.runModule(rf);
}
