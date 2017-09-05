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

#include "start-inspect.h"
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>

/**********************************************************/
bool STARTModule::configure(yarp::os::ResourceFinder &rf)
{
    moduleName = rf.check("name", yarp::os::Value("interpretSTART"), "module name (string)").asString();

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
    objectDetails = NULL;
}

/**********************************************************/
bool STARTManager::open()
{
    this->useCallback();

    //create all ports
    inSpeechPortName = "/" + moduleName + "/speech:i";
    BufferedPort<yarp::os::Bottle >::open( inSpeechPortName.c_str() );

    outSTARTPortName = "/" + moduleName + "/start:o";
    startOutPort.open( outSTARTPortName.c_str() );

    yarp::os::Network::connect("/start-speech/start:o", inSpeechPortName);

    return true;
}

/**********************************************************/
void STARTManager::close()
{
    yInfo("now closing ports...\n");
    startOutPort.close();
    BufferedPort<yarp::os::Bottle >::close();
    yInfo("finished closing the read port...\n");
}

/**********************************************************/
void STARTManager::interrupt()
{
    yInfo("cleaning up...\n");
    yInfo("attempting to interrupt ports\n");
    BufferedPort<yarp::os::Bottle >::interrupt();
    yInfo("finished interrupt ports\n");
}

/**********************************************************/
void STARTManager::onRead(yarp::os::Bottle &bot)
{
    yarp::os::Bottle &final = startOutPort.prepare();
    final.clear();

    yInfo("bot size = %d", bot.size());

    std::vector<std::string> action;
    std::string query = bot.get(0).asList()->toString();

    if (bot.size() == 1)
    {

        yInfo("Got a special query: %s ", query.c_str());
        action.push_back( bot.get(0).asList()->get(0).asString().c_str());
        yarp::os::Bottle &list = final.addList();
        yarp::os::Bottle &actList = list.addList();
        actList.addString("verb");
        action[0][0] = toupper(action[0][0]); // Make first characther upper case
        actList.addString(action[0].c_str());

    }
    else
    {
        yInfo("Got a new query: %s ", query.c_str());

        size_t sizeBot = bot.get(1).asList()->size();

        objectDetails = new Object [bot.get(0).asList()->size()];

        std::vector<std::string> subject;

        int numObjects = 0;
        int numActions = 0;
        std::string tmpString;
        std::string location;
        std::string tmpAction;

        // 1 - check for objects in list (hasdet definite)
        // 2 - add actions
        for (size_t i=0; i< sizeBot; i++)
        {

            std::string param1 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
            std::string param2 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

            if (strcmp(param1.c_str(), "hasdet") == 0 && (strcmp(param2.c_str(), "definite") == 0 || strcmp(param2.c_str(), "indefinite") == 0) ){
                objectDetails[numObjects].name = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                numObjects ++;
            }

            if (strcmp(param1.c_str(), "ismain") == 0 && strcmp(param2.c_str(), "Yes") == 0){
                action.push_back(bot.get(1).asList()->get(i).asList()->get(0).asString().c_str());
                tmpString = action[numActions].c_str();
                numActions++;

                for (int y=0; y<numActions; y++){

                    if (strcmp(action[y].c_str(), "be" ) == 0){
                        for (size_t i=0; i< sizeBot; i++){
                            std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                            std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                            if (strcmp(param1.c_str(), action[y].c_str()) == 0 && strcmp(param2.c_str(), "haslocation") == 0){
                                action.erase(std::remove(action.begin(), action.end(), tmpString), action.end());
                                action.push_back(bot.get(1).asList()->get(i).asList()->get(2).asString().c_str());
                            }
                        }
                    }
                    if (strcmp(action[y].c_str(), "isa" ) == 0){
                        for (size_t i=0; i< sizeBot; i++){
                            std::string param1 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                            std::string param2 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();
                            if (strcmp(param1.c_str(), action[y].c_str()) == 0){
                                action.erase(std::remove(action.begin(), action.end(), tmpString), action.end());
                                action.push_back(param2.c_str());
                            }
                        }
                    }

                    for (size_t i=0; i< sizeBot; i++){
                        std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                        std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                        std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

                        if (strcmp(param1.c_str(), action[y].c_str()) == 0 && (strcmp(param2.c_str(), "to") == 0 || (strcmp(param2.c_str(), "on") == 0) || (strcmp(param2.c_str(), "from") == 0))){
                            yInfo(" action %s param1 %s param2 %s param3 %s", action[y].c_str(), param1.c_str(), param2.c_str(), param3.c_str());

                            tmpAction = action[y].c_str();
                            //making first character upper case for later comparison
                            tmpAction[0] = toupper(tmpAction[0]);
                            location = param3.c_str();

                        }
                    }
                }
            }
        }

    // Fill in the subjects
    //making sure that objects and actions are the same
        if (numActions > numObjects){
            numObjects ++;
            objectDetails[numObjects-(numObjects/2)].name = objectDetails[numObjects-numObjects].name.c_str();
        }

        for (int y=0; y< numActions; y++){
            for (size_t i=0; i< sizeBot; i++){
                std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

                if (strcmp(param2.c_str(), action[y].c_str()) == 0 && strcmp(param3.c_str(), objectDetails[y].name.c_str()) == 0){
                    subject.push_back(param1.c_str());
                    break;
                }
            }
        }

        //yInfo("num objs %d", subject.size());

        if (subject.size() < 1)
            subject.push_back("you");

        subject.erase(std::remove(subject.begin(), subject.end(), ""), subject.end());
        subject.push_back("you");

        for (size_t y=0; y< subject.size(); y++){
            for (size_t i=0; i< sizeBot; i++){
                std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

                //yInfo("%s %s ", param1.c_str(), subject[y].c_str());
                if (strcmp(param1.c_str(), subject[y].c_str()) == 0 && strcmp(param2.c_str(), "hasproperty") == 0){
                    //yInfo("in here");
                    std::string adj = param3.c_str();

                    for (size_t i=0; i< sizeBot; i++){
                        std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                        std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                        std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

                        if (strcmp(param1.c_str(), adj.c_str()) == 0 && strcmp(param2.c_str(), "hascategory") == 0 && strcmp(param3.c_str(), "adj") == 0){
                            //yInfo("%s is an adjective, adding it as a parameter of %s", param1.c_str(), subject[y].c_str());
                        }
                    }
                }
            }
        }

        // if no objects are found it might be that the query is particular therefore need to check if property is available instead
        if (numObjects<1){
            for (size_t i=0; i< sizeBot; i++){
                std::string param1 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();

                if ( strcmp(param1.c_str(), "hasproperty") == 0 ){
                    objectDetails[numObjects].name = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                    numObjects ++;
                }
            }
        }

        // add properties to the objects (if any)
        for (int y=0; y< numObjects; y++){
            for (int i=0; i< sizeBot; i++)
            {
                std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
                std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
                std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();

                if (strcmp(param1.c_str(), objectDetails[y].name.c_str()) == 0 && strcmp(param2.c_str(), "hasproperty") == 0){

                    objectDetails[y].property.push_back(param3.c_str());
                }
            }
        }

        //check relatedto
        bool isProperty = false;
        for (size_t i=0; i< sizeBot; i++){
            std::string param1 = bot.get(1).asList()->get(i).asList()->get(0).asString().c_str();
            std::string param2 = bot.get(1).asList()->get(i).asList()->get(1).asString().c_str();
            std::string param3 = bot.get(1).asList()->get(i).asList()->get(2).asString().c_str();
            if (strcmp(param2.c_str(), "relatedto") == 0 )
            {
                isProperty = true;
            }
        }

        for (size_t i=0; i<numActions; i++){
            yarp::os::Bottle &list = final.addList();
            yarp::os::Bottle &subjList = list.addList();
            yarp::os::Bottle &actList = list.addList();

            subjList.addString("subject");
            subject[i][0] = toupper(subject[i][0]);
            subjList.addString(subject[i].c_str());

            actList.addString("verb");
            action[i][0] = toupper(action[i][0]); // Make first characther upper case
            actList.addString(action[i].c_str());

            yDebug() << "location is" << location.c_str() << "tmpAction is " << tmpAction.c_str() << "action is " << action[i].c_str();

            if (location.size()>0 && strcmp(tmpAction.c_str(), action[i].c_str()) == 0 ){
                yarp::os::Bottle &posList = list.addList();
                posList.addString("location");
                //location[0] = toupper(location[0]);
                posList.addString(location.c_str());
            }

            for (int i=0; i<numObjects; i++){

                //Bottle &list = final.addList();
                if (strcmp(location.c_str(), objectDetails[i].name.c_str()) == 0 ){
                    yInfo("AVOIDING object as location is equal to object %s", objectDetails[i].name.c_str());
                }
                else
                {
                    yarp::os::Bottle &objList = list.addList();
                    if (isProperty){
                        objList.addString("property");
                        isProperty = false;
                    }
                    else
                        objList.addString("object");

                    objectDetails[i].name[0] = toupper(objectDetails[i].name[0]);
                    objList.addString(objectDetails[i].name.c_str());

                    if (objectDetails[i].property.size() > 0){
                        yarp::os::Bottle &propList = list.addList();
                        propList.addString("properties");
                        yarp::os::Bottle &propElem = propList.addList();
                        for (size_t y=0; y<objectDetails[i].property.size(); y++){
                            objectDetails[i].property[y][0] = toupper(objectDetails[i].property[y][0]);
                            propElem.addString(objectDetails[i].property[y].c_str());
                        }
                    }
                }
            }
        }
    }

    //yInfo("actions is: [%s] and object is: [%s]", action.c_str(), object.c_str());

    if (final.size() < 1)
        yError("Something is wrong with the reply from START");
    else
        yInfo("output is: %s \n", final.toString().c_str());


    startOutPort.write();
}
//empty line to make gcc happy
