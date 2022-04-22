/*
 * Copyright (C) 2018 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff Laura Cavaliere Ilaria Carlini
 * email:  vadim.tikhanoff@iit.it laura.cavaliere@iit.it ilaria.carlini@iit.it
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
#include <map>

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
bool  is_changed;
using namespace std;


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
    yarp::os::Port soundOutputPort;
    enum playbackmode_t
    {
        playFromDisk=0,
        sendToPort_compressed=1,
        sendToPort_uncompressed=2
    } playbackmode;
    
public:
    /********************************************************/

    Processing( const std::string &moduleName, const std::string &language, const std::string &voice, const double &speed, const double &pitch, std::string &state, string playmode ): state(state)
    {
        this->moduleName = moduleName;
        this->language = language;
        this->voice = voice;
        this->speed = speed;
        this->pitch = pitch;
             if (playmode=="playFromDisk")             {playbackmode=playbackmode_t::playFromDisk;}
        else if (playmode=="sendToPort_compressed")    {playbackmode=playbackmode_t::sendToPort_compressed;}
        else if (playmode=="sendToPort_uncompressed")  {playbackmode=playbackmode_t::sendToPort_uncompressed;}
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
        
        if (playbackmode==playbackmode_t::sendToPort_compressed ||
            playbackmode==playbackmode_t::sendToPort_uncompressed)
        {
            soundOutputPort.open("/"+moduleName+"/sound:o");
        }
        
        return true;
    }

    /********************************************************/
    void close()
    {
        yarp::os::BufferedPort<yarp::os::Bottle >::close();
        syncPort.close();
        if (playbackmode==playbackmode_t::sendToPort_compressed ||
            playbackmode==playbackmode_t::sendToPort_uncompressed)
        {
            soundOutputPort.close();
        }
    }

    /********************************************************/
    void sendDone()
    {
        yarp::os::Bottle syncBot;
        syncBot.addString("done");
        syncPort.write(syncBot);
        yDebug() << "done querying google";
    }
    /********************************************************/
    void onRead( yarp::os::Bottle &bot )
    {
        queryGoogleSynthesis(bot);
        sendDone();
    }

   /********************************************************/
   void queryGoogleSynthesis(yarp::os::Bottle& text)
   {
       yDebug() << "in queryGoogleSynthesis";

       yDebug() << "Phrase is " << text.toString().c_str();

       std::string tmp = text.toString();

       tmp.erase(std::remove(tmp.begin(),tmp.end(),'\"'),tmp.end());

       yDebug() << "Phrase is now " << tmp.c_str();

       std::string content = tmp;

       if (content.size()>0)
       {
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
           yarp::os::Time::delay(0.2);
           grpc::Status tts_status = tts->SynthesizeSpeech(&context, request, &response);
           std::string status_string = status_code_to_string.at(tts_status.error_code());
           yInfo() << "Status string:" << status_string;
           checkState("Done");
           
           if ( tts_status.ok() )
           {
               yInfo() << "Status returned OK";
               yInfo() << "\n------Response------\n";

               if (playbackmode==playbackmode_t::playFromDisk)
               {
                   std::string file = "test.mp3";
                   std::ofstream mp3File(file, std::ios::out | std::ios::binary);

               mp3File.write( response.audio_content().data(), response.audio_content().size());

                   std::string command = "play test.mp3";// + file;

                   system(command.c_str());
               }
               else if (playbackmode==playbackmode_t::sendToPort_compressed)
               {
                    yarp::os::Value v (response.audio_content().data(), response.audio_content().size());
                    yarp::os::Bottle bot; bot.add(v);
                    soundOutputPort.write(bot);
               }
               else if (playbackmode==playbackmode_t::sendToPort_uncompressed)
               {
                    yarp::sig::Sound snd;
                    yarp::sig::file::read_bytestream(snd, response.audio_content().data(), response.audio_content().size(), ".mp3");
                    soundOutputPort.write(snd);
               }
               else
               {
                   yError() << "Invalid playbackmode";
               }
           }
           else
           {
               yError() << "Status Returned Cancelled";
               checkState("Failure_" + status_string); 
               yInfo() << tts_status.error_message();
           }

           request.release_input();
           request.release_voice();
           request.release_audio_config();

           yInfo() << "\n------finished google query------\n";
        }
        else if (content.size()==0)
        {
            checkState("Empty_input");
        }
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
    bool setLanguageCode(const std::string &languageCode)
    {
        language = languageCode;
        return true;
    }

    /********************************************************/
    bool setVoiceCode(const std::string &voiceCode)
    {
        voice = voiceCode;
        return true;
    }

    /********************************************************/
    bool setPitch(const double pitchVal)
    {
        pitch = pitchVal;
        return true;
    }

    /********************************************************/
    bool setSpeed(const double speedVal)
    {
        speed = speedVal;
        return true;
    }
    /********************************************************/
    std::string getLanguageCode()
    {
        return language;
    }

    /********************************************************/
    std::string getVoiceCode()
    {
        return voice;
    }

    /********************************************************/
    double getPitch()
    {
        return pitch;
    }

    /********************************************************/
    double getSpeed()
    {
        return speed;
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

    std::vector<std::string>    allLanguageCodes;
    std::vector<std::string>    allVoiceCodes;

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

        double speed = rf.check("speed", yarp::os::Value(1.0), "speed to use (double)").asFloat64();
        double pitch = rf.check("pitch", yarp::os::Value(0.0), "pitch to use (double)").asFloat64();

        string playmode_string = rf.check("playbackmode", yarp::os::Value("playFromDisk"), "can be one of the following: `playFromDisk`(default), `sendToPort_compressed`, `sendToPort_uncompressed`").asString();

        if (rf.check("languageCodes", "Getting language codes"))
        {
            yarp::os::Bottle &grp=rf.findGroup("languageCodes");
            int sz=grp.size()-1;

            for (int i=0; i<sz; i++)
                allLanguageCodes.push_back(grp.get(1+i).asString());
        }

        if (rf.check("voiceCodes", "Getting voice codes"))
        {
            yarp::os::Bottle &grp=rf.findGroup("voiceCodes");
            int sz=grp.size()-1;

            for (int i=0; i<sz; i++)
                allVoiceCodes.push_back(grp.get(1+i).asString());
        }

        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());
        statePort.open("/"+ moduleName + "/state:o");

        closing = false;

        processing = new Processing( moduleName, language, voice, speed, pitch, state, playmode_string);

        /* now start the thread to do the work */
        processing->open();

        if(!attach(rpcPort)) {
            yError()<<"Cannot attach to rpc port";
            return false;
        }

        return true;
    }

    /************************************************************************/
    bool attach(yarp::os::RpcServer &source)
        {
        return this->yarp().attachAsServer(source);
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
    bool say(const std::string& phrase)
    {
        yarp::os::Bottle bot;
        bot.addString(phrase);
        processing->queryGoogleSynthesis(bot);
        processing->sendDone();
        return true;
    }

    /********************************************************/
    std::string setLanguage(const std::string& languageCode, const std::string& voiceCode)
    {
        std::string returnVal = "Error, wrong language or voice code (eg: en-US en-US-Wavenet-A)";
        
        std::string language, voice;
        
        for (int i = 0; i < allLanguageCodes.size(); i++)
        {
            if (languageCode == allLanguageCodes[i])
            {
                for (int v = 0; v < allVoiceCodes.size(); v++)
                {
                    if (voiceCode == allVoiceCodes[v])
                    {
                        language = languageCode;
                        voice = voiceCode;
                        returnVal = "[ok]";
                        break;
                    }
                }
                break;
            }
        }

        if(returnVal =="[ok]")
        {
            processing->setLanguageCode(languageCode);
            processing->setVoiceCode(voiceCode);
        }
        return returnVal;
    }

    /********************************************************/
    bool setPitch(const double pitchVal)
    {
        processing->setPitch(pitchVal);
        return true;
    }

    /********************************************************/
    bool setSpeed(const double speedVal)
    {
        processing->setSpeed(speedVal);
        return true;
    }

    /********************************************************/
    std::string getLanguageCode()
    {
        return processing->getLanguageCode();
    }

    /********************************************************/
    std::string getVoiceCode()
    {
        return processing->getVoiceCode();
    }

    /********************************************************/
    double getPitch()
    {
        return processing->getPitch();
    }

    /********************************************************/
    double getSpeed()
    {
        return processing->getSpeed();
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
