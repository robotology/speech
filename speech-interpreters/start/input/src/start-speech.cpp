/*
 * Copyright (C) 2017 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
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

#include "start-speech.h"
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>

/**********************************************************/
bool STARTModule::configure(yarp::os::ResourceFinder &rf)
{
    moduleName = rf.check("name", yarp::os::Value("start-speech"), "module name (string)").asString();

    setName(moduleName.c_str());

    handlerPortName =  "/";
    handlerPortName += getName();
    handlerPortName +=  "/rpc:i";

    if (!rpcPort.open(handlerPortName.c_str()))
    {
        yError( "%s : Unable to open port %s\n", getName().c_str(), handlerPortName.c_str());
        return false;
    }

    attach(rpcPort);

    /* create the thread and pass pointers to the module parameters */
    startManager = new STARTManager( moduleName );

    /* now start the thread to do the work */
    startManager->open();

    return true ;
}

/**********************************************************/
bool STARTModule::interruptModule()
{
    rpcPort.interrupt();
    return true;
}

/**********************************************************/
bool STARTModule::close()
{
    rpcPort.close();
    yInfo( "starting the shutdown procedure\n");
    startManager->interrupt();
    startManager->close();
    yInfo( "deleting thread\n");
    delete startManager;
    yInfo( "done deleting thread\n");
    return true;
}

/**********************************************************/
bool STARTModule::updateModule()
{
    return !closing;
}

/**********************************************************/
double STARTModule::getPeriod()
{
    return 0.1;
}

/**********************************************************/
STARTManager::~STARTManager()
{

}

/**********************************************************/
STARTManager::STARTManager( const std::string &moduleName )
{
    yInfo("initialising Variables\n");
    this->moduleName = moduleName;
}

/**********************************************************/
bool STARTManager::open()
{
    this->useCallback();

    //create all ports
    inSpeechPortName = "/" + moduleName + "/speech:i";
    yarp::os::BufferedPort<yarp::os::Bottle >::open( inSpeechPortName.c_str() );

    outSTARTUrlName = "/" + moduleName + "/url";
    portURL.openFake(outSTARTUrlName); // don't use a full, real port

    outSTARTPortName = "/" + moduleName + "/start:o";
    startOutPort.open( outSTARTPortName.c_str() );

    yarp::os::Network::connect("/yarpIOS/speechPort:o", "/speechSTART/speech:i");

    return true;
}

/**********************************************************/
void STARTManager::close()
{
    yInfo("now closing ports...\n");
    startOutPort.close();
    portURL.close();
    yarp::os::BufferedPort<yarp::os::Bottle >::close();
    yInfo("finished closing the read port...\n");
}

/**********************************************************/
void STARTManager::interrupt()
{
    yInfo("cleaning up...\n");
    yInfo("attempting to interrupt ports\n");
    yarp::os::BufferedPort<yarp::os::Bottle >::interrupt();
    portURL.interrupt();
    yInfo("finished interrupt ports\n");
}

/**********************************************************/
void STARTManager::onRead(yarp::os::Bottle &bot)
{
    yarp::os::Bottle &final = startOutPort.prepare();
    final.clear();
    //yInfo("input: %s \n", bot.toString().c_str());

    if (bot.size()>0){

        std::string question = bot.toString();

        if (isalpha(question[0]))
            yInfo("avoiding erase\n");
        else{

            question.erase( remove_if(question.begin(), question.end(), aZCheck), question.end());
        }

        yInfo("cmd is %s", question.c_str());

        yarp::os::Bottle &request = final.addList();

        std::stringstream s(question);
        std::string word;

        std::string speech = "";

        bool process = true;

        //format it correctly for START
        for (int i = 0; s >> word; i++){

            if (i==0 && strcmp (word.c_str(), "no")==0)
                process=false;
            else if (i==0 && strcmp (word.c_str(), "yes")==0)
                process=false;
            else if (i==0 && strcmp (word.c_str(), "Skip")==0)
                process=false;

            speech += word.c_str();
            request.addString(word);
            speech += "+";
        }

        //clean it
        speech.erase(speech.size()-1,1);

        if (!process)
        {
            yInfo("Special cmd %s, not processing ", speech.c_str());
        }
        else
        {

            //string port = "http://start.csail.mit.edu:80/askstart.cgi?server=guest&action=parse&te=formatted-text&query=";
            std::string port = "http://start.csail.mit.edu:80/api.php?server=guest&action=parse&te=formatted-text&query=";

            std::string query = port + speech;

            portURL.addOutput(query.c_str());
            yarp::os::Bottle cmd, reply;
            cmd.addString("1"); // send any message you like, it is ignored
            portURL.write(cmd,reply);

            if (reply.size()>0){

                std::string tmp = reply.toString().c_str();

                std::string answer;
                try {
                    answer = tmp.substr(tmp.find_first_of("["), tmp.find_last_of("]"));
                }
                catch (const std::out_of_range& oor) {
                    yError("START cannot process the command %s",  tmp.c_str());
                }

                //find how many occurences of "\n"
                size_t n = std::count(answer.begin(), answer.end(), ']');

                yInfo("with %lu occurences \n", n);

                yarp::os::Bottle &tmpList = final.addList();

                for (size_t i = 0; i<n; i++)
                {
                    std::string various = answer.substr(answer.find_first_of("["), answer.find_first_of("]"));
                    various.erase(0,1);

                    //yInfo("\n tmp string is:\n %s \n", various.c_str());

                    std::stringstream str(various);
                    std::string words;

                    yarp::os::Bottle &finalTmp = tmpList.addList();

                    for (int i = 0; str >> words; i++)
                    {
                        if (words.size() > 1 )
                            words.erase( remove_if(words.begin(), words.end(), aZCheck), words.end());

                        if (isalpha(*words.c_str()))
                            finalTmp.addString( words.c_str() );
                        else
                        {
                            std::string::size_type sz;   // alias of size_t
                            int i_dec = std::stoi (words,&sz);
                            finalTmp.addInt32(i_dec);
                        }
                    }
                    answer.erase(answer.find_first_of("["), answer.find_first_of("]"));

                    answer.erase(0, answer.find_first_of("["));
                }
            }
            else{
                yError("Something is wrong with the reply from START");
            }

            yInfo(" LIST is:\n %s \n", final.toString().c_str());
        }

            startOutPort.write();
        }else

        {
            yError("Something is wrong with the query");
    }
}
//empty line to make gcc happy
