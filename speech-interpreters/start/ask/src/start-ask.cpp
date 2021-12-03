/*
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
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

#include "start-ask.h"
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <vector>

/**********************************************************/
bool STARTModule::configure(yarp::os::ResourceFinder &rf)
{
    moduleName = rf.check("name", yarp::os::Value("start-ask"), "module name (string)").asString();

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
    //rpcPort.close();
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
    BufferedPort<yarp::os::Bottle >::open( inSpeechPortName.c_str() );

    outSTARTUrlName = "/" + moduleName + "/url";
    portURL.openFake(outSTARTUrlName); // don't use a full, real port

    outSTARTPortName = "/" + moduleName + "/start:o";
    startOutPort.open( outSTARTPortName.c_str() );

    inFacePortName = "/" + moduleName + "/faces:i";
    faceInPort.open( inFacePortName.c_str() );

    return true;
}

/**********************************************************/
void STARTManager::close()
{
    yInfo("now closing ports...\n");
    startOutPort.close();
    faceInPort.close();
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
    faceInPort.interrupt();
    yInfo("finished interrupt ports\n");
}

/**********************************************************/
void STARTManager::onRead(yarp::os::Bottle &bot)
{
    yarp::os::Bottle &final = startOutPort.prepare();
    final.clear();

    if (bot.size()>0){

        std::string question = bot.toString();

        if (isalpha(question[0]))
            yInfo("avoiding erase\n");
        else
            question.erase(std::remove_if(question.begin(), question.end(), aZCheck), question.end());

        yInfo("cmd is %s", question.c_str());

        yarp::os::Bottle *faceList = faceInPort.read(false);
        std::string name;

        if (faceList!=NULL)
        {
            yarp::os::Bottle *item=faceList->get(0).asList();
            name = item->get(4).asString().c_str();
            yInfo() << "NAME IS " << name;
        }

        std::stringstream s(question);
        std::string word;

        std::string speech = "";

        bool greeting = false;

        if (strcmp (question.c_str(), "how are you") == 0 || strcmp (question.c_str(), "how are you robot") == 0 || strcmp (question.c_str(), "how are you doing") == 0)
            greeting = true;

        //format it correctly for START
        for (int i = 0; s >> word; i++){
            speech += word.c_str();

            if (strcmp (speech.c_str(), "hello" ) == 0 || strcmp (speech.c_str(), "hi" ) == 0)
                greeting = true;

            speech += "+";
        }

        //clean it
        speech.erase(speech.size()-1,1);

        //string port = "http://start.csail.mit.edu:80/askstart.cgi?te=formatted-text&query=";
        std::string port = "http://start.csail.mit.edu:80/justanswer.php?te=formatted-text&query=";

        std::string query = port + speech;

        portURL.addOutput(query.c_str());
        yarp::os::Bottle cmd, reply;
        cmd.addString("1"); // send any message you like, it is ignored
        portURL.write(cmd,reply);

        std::string html;
        std::vector<std::string>    testtags;
        std::vector<std::string>    testtext;

        yInfo("query is: %s\n", query.c_str());

        yInfo() << "\n--------------------------------Start cleaning up session-------------------------------\n";

        if (reply.size()>0){

            html = reply.toString().c_str();

            //clean the start.
            std::string tags = "<P><p>";
            std::string::size_type initial = html.find(tags);
            if (initial!=std::string::npos)
            {
                yInfo() << "Response found "<< tags << " @ " << initial << " with length " << tags.length();
                html.erase (0,initial+tags.length());
            }
            else
            {
                tags.clear();
                tags = "</P>\\n\\n";
                std::string::size_type initial = html.find(tags);
                if (initial!=std::string::npos)
                {
                    yInfo() << "Response found " << tags << " @ " << initial << " with length " << tags.length();
                    html.erase (0,initial+tags.length());
                }
                else
                {
                    tags.clear();
                    tags = "</P>";
                    std::string::size_type initial = html.find(tags);
                    if (initial!=std::string::npos)
                    {
                        yInfo() << "Response found " << tags << " @ " << initial << " with length " << tags.length();
                        html.erase (0,initial+tags.length());
                    }
                    else
                        yError() << "Cannot seem to find any info in the initial part of the response";
                }
            }

            //clean the end.
            tags.clear();
            tags="</p></P>";
            std::string::size_type final = html.find(tags);
            if (final!=std::string::npos)
            {
                yInfo() << "Complex response found "<< tags << " @ " << final << " with length " << tags.length();
                html.erase (html.begin()+final, html.end());
            }
            else
            {
                tags.clear();
                tags = "<STRONG>";
                std::string::size_type final = html.find(tags);
                if (final!=std::string::npos)
                {
                    yInfo() << "Normal response found "<< tags << " @ " << final << " with length " << tags.length();
                    html.erase (html.begin()+final, html.end());
                }
                else
                {
                    tags.clear();
                    tags = "</body>";
                    std::string::size_type final = html.find(tags);
                    if (final!=std::string::npos)
                    {
                        yInfo() << "Response found " << tags << " @ " << final << " with length " << tags.length();
                        html.erase (html.begin()+final, html.end());
                    }
                    else
                        yError() << "Cannot seem to find any info in the final part of the response";
                }
            }

            //cleaning "line feed".

            std::string::size_type n;
            tags="\\n";
            while ( (n = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << n;
                html.replace(n,tags.length()," ");
            }

            //cleaning "i know about".

            tags.clear();
            tags="I know about";
            std::string::size_type iknow = html.find(tags);
            if (iknow!=std::string::npos)
            {
                yInfo() << "Response found " << tags << " @ " << iknow << " with length " << tags.length();
                html.erase (html.begin()+iknow, html.end());
            }

            //cleaning "numeric entry".
            std::string::size_type numericEntry;
            tags="&#160;";
            while ( (numericEntry = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << numericEntry;
                html.replace(numericEntry,tags.length()," ");
            }

            //cleaning "en dash".
            std::string::size_type enDash;
            tags="&#8211;";
            while ( (enDash = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << enDash;
                html.replace(enDash,tags.length(),",");
            }

            //cleaning "en dash".
            std::string::size_type enDash2;
            tags="&#8212;";
            while ( (enDash2 = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << enDash2;
                html.replace(enDash2,tags.length(),",");
            }

            //to REMOVE   &#1071; &#1090;&#1074;&#1086;&#1081; &#1089;&#1083;&#1091;&#1075;&#1072;
            //and   &#1071; &#1090;&#1074;&#1086;&#1081; &#1088;&#1072;&#1073;&#1086;&#1090;&#1085;&#1080;&#1082;

            //cleaning "nonSpace".
            std::string::size_type nonSpace;
            tags="&nbsp;";
            while ( (nonSpace = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << nonSpace;
                html.replace(nonSpace,tags.length(),",");
            }

            //cleaning "quote".
            std::string::size_type quote;
            tags="\"";
            while ( (quote = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << quote;
                html.replace(quote,tags.length()," ");
            }

            //cleaning "forward slashes".
            std::string::size_type fslash;
            tags="/";
            while ( (fslash = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << fslash;
                html.replace(fslash,tags.length()," ");
            }

            //cleaning "back slashes".
            std::string::size_type bslash;
            tags="\\";
            while ( (bslash = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << bslash;
                html.replace(bslash,tags.length()," ");
            }

            //cleaning "dashes".
            std::string::size_type dash;
            tags="-";
            while ( (dash = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << dash;
                html.replace(dash,tags.length(),"");
            }

            //cleaning "dashes".
            std::string::size_type amp;
            tags="&amp;";
            while ( (amp = html.find(tags)) != html.npos)
            {
                yInfo() << "Cleaning up " << tags << " @ " << amp;
                html.replace(amp,tags.length(),"&");
            }

            yInfo() << "---------------------------Done the first cleaning up session---------------------------";

            for(int i=0; i<html.length(); i++)
            {
                std::string::size_type  startpos;

                startpos = html.find('<');
                if(startpos == std::string::npos)
                {
                    // no tags left only text!
                    testtext.push_back(html);
                    break;
                }

                // handle the text before the tag
                if(0 != startpos)
                {
                    testtext.push_back(html.substr(0, startpos));
                    html = html.substr(startpos, html.size() - startpos);
                    startpos = 0;
                }

                //  skip all the text in the html tag
                std::string::size_type endpos;
                for(endpos = startpos; endpos < html.size() && html[endpos] != '>'; ++endpos)
                {
                    // since '>' can appear inside of an attribute string we need
                    // to make sure we process it properly.
                    if(html[endpos] == '"')
                    {
                        endpos++;
                        while(endpos < html.size() && html[endpos] != '"')
                            endpos++;
                    }
                }

                //  Handle text and end of html that has beginning of tag but not the end
                if(endpos == html.size())
                {
                    html = html.substr(endpos, html.size() - endpos);
                    break;
                }
                else
                {
                    //  handle the entire tag
                    endpos++;
                    testtags.push_back(html.substr(startpos, endpos - startpos));
                    html = html.substr(endpos, html.size() - endpos);
                }
            }

            //yInfo() << "tags:\n-----------------";
            //for(size_t i = 0; i < testtags.size(); i++)
                //yInfo() << testtags[i];

            html.clear();
            for(size_t i = 0; i < testtext.size(); i++)
                html = html + testtext[i];

            yInfo() << "\n-------------------------------Starting specific cleaning-------------------------------";
            yInfo() << "--------------------------------- check for [ ] removal ----------------------------------";

            std::string first = "[";
            std::string second = "]";

            removetags(first, second, html);

            yInfo() << "--------------------------------- check for ( ) removal ----------------------------------";
            first.clear();
            second.clear();
            first = "(";
            second = ")";

            removetags(first, second, html);
        }
        else
            yError("Something is wrong with the reply from START");
        if (greeting && name.size()>0)
            html = name + ", " + html;

        final.addString(html.c_str());
        yInfo() << "\n\n\n";
        yInfo() << "The original answer was:\n " << reply.toString().c_str();
        yInfo() << "\n\n\n";
        yInfo() << "The answer is:\n " << final.toString().c_str();
        startOutPort.write();
    }else
        yError("Something is wrong with the query");
}

/**********************************************************/
void STARTManager::removetags(std::string &first, std::string &second, std::string &text)
{
    std::string tags = first;

    std::vector<size_t> startPos;
    std::vector<size_t> endPos;

    size_t pos = text.find(tags);
    while(pos != std::string::npos)
    {
        yInfo() << "found " << tags << " @ " << pos;
        startPos.push_back(pos);
        pos = text.find(tags, pos+1);
    }

    tags.clear();
    tags=second;
    pos = 0;
    pos = text.find(tags);
    while(pos != std::string::npos)
    {
        yInfo() << "found " << tags << " @ " << pos;
        endPos.push_back(pos);
        pos = text.find(tags, pos+1);
    }

    int shift = 0;

    std::vector<size_t> tmpstartPos = startPos;
    std::vector<size_t> tmpendPos = endPos;

    if (tmpstartPos.size() == tmpendPos.size()) //check for consistencies
    {
        for (int i =0; i<tmpstartPos.size(); i++)
        {
            //check if tag occurs withing another tag

            if ( i != tmpstartPos.size()-1 && tmpendPos[i] > tmpstartPos[i+1] )
            {
                startPos.erase(startPos.begin()+1);
                endPos.erase(endPos.begin());
            }
        }

        for (int i =0; i<startPos.size(); i++)
        {
            if (i>0)
                shift = shift + (endPos[i-1] - startPos[i-1] + 1 );

            yInfo() << "deleting pos " << startPos[i] << " until " << endPos[i] << " with shift " << shift;

            text.erase (text.begin()+(startPos[i]-shift), text.begin()+(endPos[i] + 1 - shift));
        }
    }
}

//empty line to make gcc happy
