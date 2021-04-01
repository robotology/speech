/*
 * Copyright (C) 2018 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff
 * email:  vadim.tikhanoff@iit.it
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

#include <vector>
#include <iostream>
#include <deque>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <ctime>

#include <fstream>
#include <iterator>
#include <string>

#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Time.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Semaphore.h>
#include <yarp/sig/SoundFile.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/sig/SoundFile.h>

#include <grpc++/grpc++.h>
#include "google/cloud/speech/v1/cloud_speech.grpc.pb.h"

#include "googleSpeech_IDL.h"

using google::cloud::speech::v1::RecognitionConfig;
using google::cloud::speech::v1::Speech;
using google::cloud::speech::v1::RecognizeRequest;
using google::cloud::speech::v1::RecognizeResponse;
std::mutex mtx;
bool is_changed;

/********************************************************/
class Processing : public yarp::os::TypedReaderCallback<yarp::sig::Sound>
{
    std::string moduleName;
    yarp::os::RpcServer handlerPort;
    yarp::os::BufferedPort<yarp::sig::Sound> port;
    yarp::os::BufferedPort<yarp::os::Bottle> targetPort;
    yarp::os::RpcClient audioCommand;
    std::string &state; 
    std::int64_t &elapsed_seconds; 

    std::deque<yarp::sig::Sound> sounds;

    int samples;
    int channels;
    int padding;
    bool getSounds;
    bool sendForQuery;
    std::string language;
    int sample_rate;

    std::chrono::time_point<std::chrono::system_clock> start, end;

public:
    /********************************************************/

    Processing( const std::string &moduleName, const std::string &language, const int sample_rate,  std::string &state, std::int64_t &elapsed_seconds ) : state(state), elapsed_seconds(elapsed_seconds)
    {
        this->moduleName = moduleName;
        yInfo() << "language " << language;
        yInfo() << "sample_rate " << sample_rate;
        this->language = language;
        this->sample_rate = sample_rate;

        port.useCallback(*this);
        port.setStrict();
        samples = 0;
        channels = 0;
        padding = 0;
        getSounds = false;
        sendForQuery = false;
    }

    /********************************************************/
    ~Processing()
    {

    };

    /********************************************************/
    bool open()
    {
        port.setStrict(true);

        port.open("/" + moduleName + "/sound:i");
        targetPort.open("/"+ moduleName + "/result:o");
        audioCommand.open("/"+ moduleName + "/commands:rpc");

        //yarp::os::Network::connect("/microphone/audio:o", port.getName());
        //yarp::os::Network::connect(audioCommand.getName(), "/microphone/rpc");

        return true;
    }

    /********************************************************/
    void close()
    {
        port.close();
        targetPort.close();
        audioCommand.close();
    }

    /********************************************************/
    using yarp::os::TypedReaderCallback<yarp::sig::Sound>::onRead;
    void onRead( yarp::sig::Sound& sound ) override
    {    
        std::lock_guard<std::mutex> lg(mtx); 

        if(getSounds)
        {   
            int ct = port.getPendingReads();
            while (ct>padding) 
            {
                ct = port.getPendingReads();
                yWarning() << "Dropping sound packet -- " << ct << " packet(s) behind";
                port.read();
            }
            collectFrame(sound);
        }

        if(sendForQuery)
        {  
            //unpack sound
            yarp::sig::Sound total;
            total.resize(samples,channels);
            long int at = 0;
            while (!sounds.empty()) {
                yarp::sig::Sound& tmp = sounds.front();
                
                yDebug() << "channels " << channels;
                yDebug() << "samples " << tmp.getSamples();
                yDebug() << "values " << tmp.get(0,0);
                
                for (int i=0; i<channels; i++) {
                    for (int j=0; j<tmp.getSamples(); j++) {
                        total.set(tmp.get(j,i),at+j,i);
                    }
                }
                total.setFrequency(tmp.getFrequency());
                at += tmp.getSamples();
                sounds.pop_front();
            }
            yarp::os::Bottle &outTargets = targetPort.prepare();
            
            /*std::string name = "test.wav";
            bool ok = yarp::sig::file::write(total,"test.wav");
            if (ok) {
                yDebug("Wrote audio to %s\n", name.c_str());
            }*/
            
            yarp::os::Bottle cmd, rep;
            cmd.addString("stop");
            if (audioCommand.write(cmd, rep))
            {
                yDebug() << "cmd.addString(stop)" << rep.toString().c_str();
            }
            
            outTargets = queryGoogle(total);

            sendForQuery = false;
            samples = 0;
            channels = 0;
            targetPort.write();
            yDebug() << "done querying google";
        }
        yarp::os::Time::yield();
    }

    /********************************************************/
    void collectFrame(yarp::sig::Sound& sound) 
    {
        sounds.push_back(sound);
        samples += sound.getSamples();
        channels = sound.getChannels();
        yDebug() <<  (long int) sounds.size() << "sound frames buffered in memory ( " << (long int) samples << " samples)";
    }

    /********************************************************/
    yarp::os::Bottle queryGoogle(yarp::sig::Sound& sound)
    {
        RecognizeRequest request;

        yDebug() << "in queryGoogle";
        yDebug() << "language" << language;
        yarp::os::Bottle b;
        b.clear();
        auto creds = grpc::GoogleDefaultCredentials();
        auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
        std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));

        setArguments(request.mutable_config());

        yInfo() << "getFrequency " << sound.getFrequency();
        yInfo() << "getSamples " << sound.getSamples();
        yInfo() << "getChannels " << sound.getChannels();
        yInfo() << "getBytesPerSamples " << sound.getBytesPerSample();
        
        auto vec_i = sound.getNonInterleavedAudioRawData();
        //auto vec_i = sound.getInterleavedAudioRawData();
        auto s1 = std::vector<short>(vec_i.begin(), vec_i.end());
        
        yInfo() << "AudioRawData s1.size()" << s1.size();

        request.mutable_audio()->mutable_content()->assign((char*)s1.data(), s1.size()*2);
        
        end = std::chrono::system_clock::now();

        double start_elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();

        yInfo() << "From start to mutable audio " << start_elapsed_seconds / 1000 << " seconds passed";

        grpc::ClientContext context;
        RecognizeResponse response;

        start = std::chrono::system_clock::now();
        grpc::Status rpc_status = speech->Recognize(&context, request, &response);
        end = std::chrono::system_clock::now();
        checkState("Done");

        elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();
        yInfo() << "Sending to google took " << elapsed_seconds << " ms";

        if (!rpc_status.ok()) {
            // Report the RPC failure.
            yInfo() << rpc_status.error_message();
            b.clear();
            checkState("Failure");
        }
        
        yInfo() << "Size of response " << response.results_size();
        
        // Dump the transcript of all the results.
        for (int r = 0; r < response.results_size(); ++r) 
        {
            auto result = response.results(r);
            for (int a = 0; a < result.alternatives_size(); ++a) 
            {
                auto alternative = result.alternatives(a);
                yInfo() << alternative.confidence();
                yInfo() << alternative.transcript();
                b.addString(alternative.transcript());
            }
        }
        return b;
    }

    /********************************************************/
    void setArguments(RecognitionConfig* config)
    {
        config->set_language_code(language.c_str());
        config->set_sample_rate_hertz(sample_rate);
        config->set_encoding(RecognitionConfig::LINEAR16);
    }

    /********************************************************/
    bool start_acquisition()
    {           
        std::lock_guard<std::mutex> lg(mtx); 
        yarp::os::Bottle cmd, rep;
        //cmd.addVocab(yarp::os::Vocab::encode("start"));
        cmd.addString("start");
        if (audioCommand.write(cmd, rep))
        {
            yDebug() << "cmd.addString(start)" << rep.toString().c_str();
        }
        
        start = std::chrono::system_clock::now();
        getSounds = true;
        checkState("Listening");
        return true;
    }

    /********************************************************/
    bool stop_acquisition()
    {   
        std::lock_guard<std::mutex> lg(mtx); 
        /*yarp::os::Bottle cmd, rep;
        cmd.addString("stop");
        if (audoCommand.write(cmd, rep))
        {
            yDebug() << "cmd.addString(stop)" << rep.toString().c_str();
        }*/
        getSounds = false;
        sendForQuery = true;
        checkState("Busy");
        return true;
    } 
    /********************************************************/
    bool checkState(std::string new_state)
    {   
        if(new_state!=state){
            is_changed=true;
            state=new_state;
        }
        else{
            is_changed=false;
        }
        return is_changed;
    }
};

/********************************************************/
class Module : public yarp::os::RFModule, public googleSpeech_IDL
{
    yarp::os::ResourceFinder    *rf;
    yarp::os::RpcServer         rpcPort;
    std::string state;
    std::int64_t elapsed_seconds;
    yarp::os::BufferedPort<yarp::os::Bottle> statePort;

    Processing                  *processing;
    friend class                processing;

    bool                        closing;

    /********************************************************/
    bool attach(yarp::os::RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

public:

    /********************************************************/
    bool configure(yarp::os::ResourceFinder &rf)
    {
        this->rf=&rf;
        this->state="Ready";
        this->elapsed_seconds=0;
        std::string moduleName = rf.check("name", yarp::os::Value("yarp-google-speech"), "module name (string)").asString();
        std::string language = rf.check("language_code", yarp::os::Value("en-US"), "language (string)").asString();
        int sample_rate = rf.check("sample_rate_hertz", yarp::os::Value(16000), "sample rate (int)").asInt();

        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());
        statePort.open("/"+ moduleName + "/state:o");

        closing = false;

        processing = new Processing( moduleName, language, sample_rate,  state, elapsed_seconds );

        /* now start the thread to do the work */
        processing->open();

        attach(rpcPort);

        return true;
    }

    /**********************************************************/
    bool close()
    {   
        //statePort.close();
        processing->close();
        delete processing;
        return true;
    }

    /**********************************************************/
    bool start()
    {
        processing->start_acquisition();
        return true;
    }

    /**********************************************************/
    bool stop()
    {
        processing->stop_acquisition();
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /********************************************************/
    bool quit()
    {
        closing=true;
        return true;
    }

    /********************************************************/
    bool updateModule()
    {   
        if(is_changed){
            is_changed=false;
            yarp::os::Bottle &outTargets = statePort.prepare();   
            outTargets.clear();  
            outTargets.addString(state);
            yDebug() << "outTarget:" << outTargets.toString().c_str();
            statePort.write();
        }    
        return !closing;
    }
       /********************************************************/
    std::string getState()
    {  
        return state;
    }
 
    /********************************************************/
    std::int64_t getProcessingTime()
    {  
        return elapsed_seconds;
    }
};

/********************************************************/
int main(int argc, char *argv[])
{
    yarp::os::Network::init();

    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("YARP server not available!");
        return 1;
    }

    Module module;
    yarp::os::ResourceFinder rf;

    rf.setVerbose( true );
    rf.setDefaultContext( "googleSpeech" );
    rf.setDefaultConfigFile( "config.ini" );
    rf.setDefault("name","googleSpeech");
    rf.configure(argc,argv);

    return module.runModule(rf);
}
