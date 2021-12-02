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

#ifndef __ICUB_STARTASK_MOD_H__
#define __ICUB_STARTASK_MOD_H__

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Thread.h>
#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>


class STARTManager : public yarp::os::BufferedPort<yarp::os::Bottle>
{
private:

    std::string moduleName;                                             //string containing module name
    std::string inSpeechPortName;                                       //string containing speech input port name
    std::string outSTARTPortName;                                       //string containing start output port name
    std::string inFacePortName;                                         //string containing face input port name

    std::string outSTARTUrlName;                                        //string containing start output port name

    yarp::os::Port portURL;

    yarp::os::BufferedPort<yarp::os::Bottle>    speechInPort;           //input port containing speech
    yarp::os::BufferedPort<yarp::os::Bottle>    startOutPort;           //output port containing start
    yarp::os::BufferedPort<yarp::os::Bottle>    faceInPort;             //

public:
    /**
     * constructor
     * @param moduleName is passed to the thread in order to initialise all the ports correctly (default yuvProc)
     * @param imgType is passed to the thread in order to work on YUV or on HSV images (default yuv)
     */
    STARTManager( const std::string &moduleName );
    ~STARTManager();

    bool    open();
    void    close();
    void    onRead( yarp::os::Bottle &bot );
    void    interrupt();
    void    removetags(std::string &first, std::string &second, std::string &text);

    static bool aZCheck(char c)
    {
        if (isspace(c))
            return isalpha(c);
        else
            return !isalpha(c);
    }
};
/**********************************************************/

class STARTModule:public yarp::os::RFModule
{
    /* module parameters */
    std::string             moduleName;
    std::string             handlerPortName;
    yarp::os::RpcServer     rpcPort;

    /* pointer to a new thread */
    STARTManager            *startManager;
    bool                    closing;

public:

    bool configure(yarp::os::ResourceFinder &rf); // configure all the module parameters and return true if successful
    bool interruptModule();                       // interrupt, e.g., the ports
    bool close();                                 // close and shut down the module

    double getPeriod();
    bool updateModule();
};


#endif
//empty line to make gcc happy
