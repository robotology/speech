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
#include <map>

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

static const std::map<grpc::StatusCode, std::string> status_code_to_string {
    {grpc::OK, "ok"},
    {grpc::CANCELLED, "cancelled"},
    {grpc::UNKNOWN, "unknown"},
    {grpc::INVALID_ARGUMENT, "invalid_argument"},
    {grpc::DEADLINE_EXCEEDED, "deadline_exceeded"},
    {grpc::NOT_FOUND, "not_found"},
    {grpc::ALREADY_EXISTS, "already_exists"},
    {grpc::PERMISSION_DENIED, "permission_denied"},
    {grpc::UNAUTHENTICATED, "unauthenticated"},
    {grpc::RESOURCE_EXHAUSTED , "resource_exhausted"},
    {grpc::FAILED_PRECONDITION, "failed_precondition"},
    {grpc::ABORTED, "aborted"},
    {grpc::OUT_OF_RANGE, "out_of_range"},
    {grpc::UNIMPLEMENTED, "unimplemented"},
    {grpc::INTERNAL, "internal"},
    {grpc::UNAVAILABLE, "unavailable"},
    {grpc::DATA_LOSS, "data_loss"},
    {grpc::DO_NOT_USE, "do_not_use"}
};
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

    bool uniqueSound;
    
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
        uniqueSound = false;
    }

    /********************************************************/
    void setUsingUniqueSound()
    {
        uniqueSound = true;
        getSounds = true;
        sendForQuery = true;
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
                    
            yarp::os::Bottle cmd, rep;
            cmd.addString("stop");
            if (audioCommand.write(cmd, rep))
            {
                yDebug() << "cmd.addString(stop)" << rep.toString().c_str();
            }
            
            outTargets = queryGoogle(total);
        
            if (!uniqueSound)
                sendForQuery = false;

            samples = 0;
            channels = 0;
            if(outTargets.size()>0){
                targetPort.write();
            }
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
        auto s1 = std::vector<short>(vec_i.begin(), vec_i.end());
        
        yInfo() << "AudioRawData s1.size()" << s1.size();

        request.mutable_audio()->mutable_content()->assign((char*)s1.data(), s1.size()*2);
        
        end = std::chrono::system_clock::now();

        double start_elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();

        yInfo() << "From start to mutable audio " << start_elapsed_seconds / 1000 << " seconds passed";

        grpc::ClientContext context;
        RecognizeResponse response;

        start = std::chrono::system_clock::now();
        if (uniqueSound){
             checkState("Busy");
        }
        yarp::os::Time::delay(0.2);
        grpc::Status rpc_status = speech->Recognize(&context, request, &response);
        std::string status_string = status_code_to_string.at(rpc_status.error_code());
        end = std::chrono::system_clock::now();

        elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();
        yInfo() << "Sending to google took " << elapsed_seconds << " ms";

        if (!rpc_status.ok()) {
            // Report the RPC failure.
            yInfo() << rpc_status.error_message();
            b.clear();
            checkState("Failure_" + status_string); 
        }
        else{
            yInfo() << "Size of response " << response.results_size();
            if(response.results_size()>0){
                checkState("Done");

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
            }
            else{
                checkState("Empty");
            }
        }
        return b;
    }

    /********************************************************/
    bool setLanguageCode(const std::string &languageCode)
    {
        language = languageCode;
        return true;
    }

    /********************************************************/
    std::string getLanguageCode()
    {
        return language;
    }

    /********************************************************/
    void setArguments(RecognitionConfig* config)
    {
        config->set_language_code(language.c_str());
        config->set_sample_rate_hertz(sample_rate);
        config->set_encoding(RecognitionConfig::LINEAR16);

        config->set_use_enhanced(true); // Can be used with the correct model. If true but no model specified, it does nothing.
        auto metadata = config->mutable_metadata();

        metadata->set_microphone_distance(google::cloud::speech::v1::RecognitionMetadata_MicrophoneDistance_MIDFIELD);
        metadata->set_recording_device_type(google::cloud::speech::v1::RecognitionMetadata_RecordingDeviceType_OTHER_INDOOR_DEVICE);
        metadata->set_interaction_type(google::cloud::speech::v1::RecognitionMetadata_InteractionType_VOICE_COMMAND);
        metadata->set_original_media_type(google::cloud::speech::v1::RecognitionMetadata_OriginalMediaType_AUDIO);
    }

    /********************************************************/
    bool start_acquisition()
    {           
        std::lock_guard<std::mutex> lg(mtx); 
        yarp::os::Bottle cmd, rep;
        //cmd.addVocab32("start");
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
        
        if (!uniqueSound) 
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
    bool                        uniqueSound;
    std::vector<std::string>    allLanguageCodes;

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
        uniqueSound = false;

        std::string moduleName = rf.check("name", yarp::os::Value("googleSpeech"), "module name (string)").asString();
        std::string language = rf.check("language_code", yarp::os::Value("en-US"), "language (string)").asString();
        int sample_rate = rf.check("sample_rate_hertz", yarp::os::Value(16000), "sample rate (int)").asInt32();
        
        if (rf.check("uniqueSound", "use a yarp::sig::Sound instead of a microphone"))
            uniqueSound = true;
        
        if (rf.check("languageCodes", "Getting language codes"))
        {
            yarp::os::Bottle &grp=rf.findGroup("languageCodes");
            int sz=grp.size()-1;

            for (int i=0; i<sz; i++)
                allLanguageCodes.push_back(grp.get(1+i).asString());
        }

        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());
        statePort.open("/"+ moduleName + "/state:o");

        closing = false;

        processing = new Processing( moduleName, language, sample_rate,  state, elapsed_seconds);

        /* now start the thread to do the work */
        processing->open();

        if (uniqueSound)
            processing->setUsingUniqueSound();
        
        attach(rpcPort);

        return true;
    }

    /********************************************************/
    bool setLanguage(const std::string& languageCode)
    {
        bool returnVal = false;
        
        std::string language;
        
        for (int i = 0; i < allLanguageCodes.size(); i++)
        {
            if (languageCode == allLanguageCodes[i])
            {
                language = languageCode;
                processing->setLanguageCode(languageCode);
                returnVal = true;
                break;
            }
        }

        return returnVal;
    }

    /********************************************************/
    std::string getLanguageCode()
    {
        return processing->getLanguageCode();
    }


    /**********************************************************/
    bool close()
    {   
        statePort.close();
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
