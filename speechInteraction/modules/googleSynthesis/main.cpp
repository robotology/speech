/*
 * Copyright (C) 2018 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff Laura Cavaliere
 * email:  vadim.tikhanoff@iit.it laura.cavaliere@iit.it
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

#include <fstream>
#include <iterator>
#include <string>

#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Time.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Semaphore.h>
#include <yarp/sig/SoundFile.h>
#include <yarp/dev/PolyDriver.h>

#include <grpc++/grpc++.h>
#include "google/cloud/language/v1/language_service.grpc.pb.h"
#include "google/cloud/texttospeech/v1/cloud_tts.grpc.pb.h"

#include "googleSynthesis_IDL.h"

using namespace google::cloud::language::v1;
using namespace google::cloud::texttospeech::v1;
bool is_changed;

/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::os::Bottle>
{
    std::string moduleName;
    std::string language;
    std::string voice;
    std::string &state;
    double speed;
    double pitch;
    yarp::os::RpcServer handlerPort;
    yarp::os::Port syncPort;

public:
    /********************************************************/

    Processing( const std::string &moduleName, const std::string &language, const std::string &voice, const double &speed, const double &pitch, std::string &state ): state(state)
    {
        this->moduleName = moduleName;
        this->language = language;
        this->voice = voice;
        this->speed = speed;
        this->pitch = pitch;
    }

    /********************************************************/
    ~Processing()
    {

    };

    /********************************************************/
    bool open()
    {
        this->useCallback();
        yarp::os::BufferedPort<yarp::os::Bottle >::open( "/" + moduleName + "/text:i" );
        syncPort.open( "/" + moduleName + "/sync:o" );
        return true;
    }

    /********************************************************/
    void close()
    {
        yarp::os::BufferedPort<yarp::os::Bottle >::close();
        syncPort.close();
    }

    /********************************************************/
    void onRead( yarp::os::Bottle &bot )
    {
        queryGoogleSynthesis(bot);
        yarp::os::Bottle syncBot;
        syncBot.addString("done");
        syncPort.write(syncBot);
        yDebug() << "done querying google";
    }

/********************************************************/
   void queryGoogleSynthesis(yarp::os::Bottle& text)
   {
       yDebug() << "in queryGoogleSynthesis";

       yDebug() << "Phrase is " << text.toString().c_str();

       std::string tmp = text.toString();

       std::map< std::string, std::string> dictionary;
       dictionary.insert ( std::pair<std::string,std::string>("á","a") );
       dictionary.insert ( std::pair<std::string,std::string>("à","a") );
       dictionary.insert ( std::pair<std::string,std::string>("é","e") );
       dictionary.insert ( std::pair<std::string,std::string>("è","e") );
       dictionary.insert ( std::pair<std::string,std::string>("í","i") );
       dictionary.insert ( std::pair<std::string,std::string>("ó","o") );
       dictionary.insert ( std::pair<std::string,std::string>("ú","u") );
       dictionary.insert ( std::pair<std::string,std::string>("ñ","n") );

       std::string tmp2 = tmp;
       std::string strAux;
       for (auto it= dictionary.begin(); it != dictionary.end(); it++)
       {
           tmp2=(it->first);
           std::size_t found=tmp.find_first_of(tmp2);


           while (found!=std::string::npos)
           {
               yError() << "in found" << found;
               strAux=(it->second);
               tmp.erase(found,tmp2.length());
               tmp.insert(found,strAux);
               found=tmp.find_first_of(tmp2,found+1);
           }
       }

       yDebug() << "Phrase is now " << tmp.c_str();
       tmp.erase(std::remove(tmp.begin(),tmp.end(),'\"'),tmp.end());

       yDebug() << tmp.size();
       yDebug() << std::isalnum(tmp[1]);

       if (tmp.size() > 1 && std::isalnum(tmp[0])==0)
           tmp = tmp.substr(1, tmp.size() - 2);

       yDebug() << "Phrase is now " << tmp.c_str();

       std::string content = tmp;

       SynthesizeSpeechRequest request;
       SynthesizeSpeechResponse response;

       grpc::Status status;
       grpc::ClientContext context;

       auto creds = grpc::GoogleDefaultCredentials();
       auto channel = grpc::CreateChannel("texttospeech.googleapis.com", creds);
       std::unique_ptr<TextToSpeech::Stub> tts(TextToSpeech::NewStub(channel));

       AudioConfig audio_config;
       VoiceSelectionParams params;

       SynthesisInput input;
       input.set_text(content);

       audio_config.set_audio_encoding(MP3);
       params.set_language_code(language);
       params.set_ssml_gender(NEUTRAL);
       params.set_name(voice);
       audio_config.set_speaking_rate(speed);
       audio_config.set_pitch(pitch);

       request.set_allocated_input(&input);
       request.set_allocated_voice(&params);
       request.set_allocated_audio_config(&audio_config);

       checkState("Busy");
       grpc::Status tts_status = tts->SynthesizeSpeech(&context, request, &response);
       checkState("Done");

       if ( tts_status.ok() )
       {
           yInfo() << "Status returned OK";
           yInfo() << "\n------Response------\n";

           std::string file = "test.mp3";
           std::ofstream mp3File(file, std::ios::out | std::ios::binary);

           mp3File.write( response.audio_content().data(), response.audio_content().size());

           std::string command = "play test.mp3";// + file;

           system(command.c_str());

       } else if ( !status.ok() ){
           yError() << "Status Returned Canceled";
           checkState("Failure");
       }
       request.release_input();
       request.release_voice();
       request.release_audio_config();

       yInfo() << "\n------dgb1------\n";
   }

    /********************************************************/
    bool start_acquisition()
    {
        return true;
    }

    /********************************************************/
    bool stop_acquisition()
    {
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
class Module : public yarp::os::RFModule, public googleSynthesis_IDL
{
    yarp::os::ResourceFinder    *rf;
    yarp::os::RpcServer         rpcPort;
    yarp::os::BufferedPort<yarp::os::Bottle> statePort;
    std::string state;
     
    Processing                  *processing;
    friend class                processing;

    bool                        closing;

    /********************************************************/

public:

    /********************************************************/
    bool configure(yarp::os::ResourceFinder &rf)
    {
        this->rf=&rf;
        this->state="Ready";
        std::string moduleName = rf.check("name", yarp::os::Value("googleSynthesis"), "module name (string)").asString();

        std::string language = rf.check("language", yarp::os::Value("en-US"), "language to use (string)").asString();
        std::string voice = rf.check("voice", yarp::os::Value("en-US-Wavenet-D"), "voice to use (string)").asString();

        double speed = rf.check("speed", yarp::os::Value(1.0), "speed to use (double)").asDouble();
        double pitch = rf.check("pitch", yarp::os::Value(0.0), "pitch to use (double)").asDouble();

        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());
        statePort.open("/"+ moduleName + "/state:o");

        closing = false;

        processing = new Processing( moduleName, language, voice, speed, pitch, state );

        /* now start the thread to do the work */
        processing->open();

        if(!attach(rpcPort)) {
            yError()<<"Cannot attach to rpc port";
            return false;
        }

        return true;
    }

    bool respond(const yarp::os::Bottle& command, yarp::os::Bottle& reply) override
    {
        auto cmd0=command.get(0).asString();
        if (cmd0!="say")
        {
            reply.addString("Command not recognized, please specify \"say <sentence>\"");
            return false;
        }

        if (command.size()>1)
        {
           yarp::os::Bottle sentence_bot;
           sentence_bot.addString(command.get(1).asString());
           processing->queryGoogleSynthesis(sentence_bot);
           reply.addString("ack");
        }

        return yarp::os::RFModule::respond(command,reply);
    }
    /**********************************************************/
    bool close()
    {  
        statePort.close();
        processing->close();
        delete processing;
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
    rf.setDefaultContext( "googleSynthesis" );
    rf.setDefaultConfigFile( "config.ini" );
    rf.setDefault("name","googleSynthesis");
    rf.configure(argc,argv);

    return module.runModule(rf);
}
