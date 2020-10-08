/*
 * Copyright (C) 2018 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Ilaria Carlini Laura Cavaliere Vadim Tikhanoff 
 * email:  ilaria.carlini@iit.it laura.cavaliere@iit.it vadim.tikhanoff@iit.it 
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
#include "google/cloud/dialogflow/cx/v3beta1/agent.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/audio_config.grpc.pb.h"
//#include "google/cloud/dialogflow/cx/v3beta1/context.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/entity_type.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/intent.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/session_entity_type.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/session.grpc.pb.h"
#include "google/cloud/dialogflow/cx/v3beta1/webhook.grpc.pb.h"


#include "googleDialog_IDL.h"

using namespace google::cloud::language::v1;
using namespace google::cloud::texttospeech::v1;
using namespace google::cloud::dialogflow::cx::v3beta1;

/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::os::Bottle>
{
    std::string moduleName;  
    std::string session_id;  
    std::string project_id;  
    yarp::os::RpcServer handlerPort;
    yarp::os::BufferedPort<yarp::os::Bottle> targetPort;

public:
    /********************************************************/

    Processing( const std::string &moduleName, const std::string project_id)
    {
        this->moduleName = moduleName;
        this->session_id = getRandSession();
        this->project_id = project_id;
    }

    /********************************************************/
    ~Processing()
    {

    };
    /********************************************************/
    std::string getRandSession() {
        std::string session;
        for(int i = 0; i<16; i++) {
            session += std::to_string(rand() % 10);
        }
        return session;
    }
    /********************************************************/
    bool open()
    {
        this->useCallback();
        yarp::os::BufferedPort<yarp::os::Bottle >::open( "/" + moduleName + "/text:i" );
        targetPort.open("/"+ moduleName + "/result:o");

        return true;
    }

    /********************************************************/
    void close()
    {
        yarp::os::BufferedPort<yarp::os::Bottle >::close();
        targetPort.close();
    }

    /********************************************************/
    void onRead( yarp::os::Bottle &bot)
    {   

        yarp::os::Bottle &outTargets = targetPort.prepare();

        outTargets.clear();
        outTargets = queryGoogleDialog(bot);
        yDebug() << "bottle" << outTargets.toString();
        targetPort.write();

        yDebug() << "done querying google";
    }

/********************************************************/
    yarp::os::Bottle queryGoogleDialog(yarp::os::Bottle& bottle)
    {
       std::string text = bottle.toString();

       google::cloud::dialogflow::cx::v3beta1::TextInput text_input;
       google::cloud::dialogflow::cx::v3beta1::QueryInput query_input;

       std::string language_code = "en-English";
       text_input.set_text(text.c_str());
       query_input.set_allocated_text(&text_input);
       query_input.set_language_code(language_code);

       grpc::Status status;
       grpc::ClientContext context;

       DetectIntentRequest request;
       DetectIntentResponse response;
       
       request.set_session("projects/"+project_id+"/locations/global/agents/10b09025-da6c-41a5-a5d1-65ad9ae6ab68/environments/draft/sessions/"+session_id);
       request.set_allocated_query_input(&query_input);
       
       yDebug() << "End-user expression: " << request.query_input().text().text();

       auto creds = grpc::GoogleDefaultCredentials();
       auto channel = grpc::CreateChannel("dialogflow.googleapis.com", creds);
       std::unique_ptr<Sessions::Stub> dialog (Sessions::NewStub(channel));

       grpc::Status dialog_status = dialog->DetectIntent(&context, request, &response);

       yarp::os::Bottle result;
       result.clear();
       if ( dialog_status.ok() ) {

           yInfo() << "Status returned OK";
           yInfo() << "\n------Response------\n";

           result.addString(response.query_result().response_messages().Get(0).text().text().Get(0).c_str());
           yDebug() << "result bottle" << result.toString();

      } else if ( !dialog_status.ok() ) {
            yError() << "Status Returned Canceled";
      }
      request.release_query_input();
      query_input.release_text();
      
      return result;
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

};

/********************************************************/
class Module : public yarp::os::RFModule, public googleDialog_IDL
{
    yarp::os::ResourceFinder    *rf;
    yarp::os::RpcServer         rpcPort;

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
        std::string moduleName = rf.check("name", yarp::os::Value("googleDialog"), "module name (string)").asString();
        std::string project_id = rf.check("project", yarp::os::Value("1"), "id of the project").asString();
        
        yDebug() << "this is the project" << rf.check("project");
        yDebug() << "Module name" << moduleName;
        yDebug() << "project id" << project_id;
        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());

        closing = false;

        processing = new Processing( moduleName, project_id );

        /* now start the thread to do the work */
        processing->open();

        attach(rpcPort);

        return true;
    }

    /**********************************************************/
    bool close()
    {
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
    rf.setDefaultContext( "googleDialog" );
    rf.setDefaultConfigFile( "config.ini" );
    rf.setDefault("name","googleDialog");
    rf.configure(argc,argv);

    return module.runModule(rf);
}
