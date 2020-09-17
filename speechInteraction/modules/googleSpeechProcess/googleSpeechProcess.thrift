# Copyright (C) 2019 Fondazione Istituto Italiano di Tecnologia (IIT)  
# All Rights Reserved.
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
#
# googleTextProcess.thrift

/**
* googleSpeechProcess_IDL
*
* IDL Interface to google cloud speech processing.
*/

struct Bottle {
} (
  yarp.name = "yarp::os::Bottle"
  yarp.includefile = "yarp/os/Bottle.h"
)

service googleSpeechProcess_IDL
{

    /**
     * Starts the sound acquisition
     * @return true/false on success/failure
     */
    bool start();

    /**
     * Stops the sound acquisition
     * @return true/false on success/failure
     */
    bool stop();

    /**
     * Quit the module.
     * @return true/false on success/failure
     */
    bool quit();

    /**
     * Get the dependency of the speech analysis for each word of the text.
     * @return a bottle
     */
    Bottle get_dependency();

  
    /**
     * Get the parse label of the speech analysis for each word of the text.
     * @return a bottle
     */
    Bottle get_parseLabel();

        
    /**
     * Get the Part Of Speech of the speech analysis for each word of the text.
     * @return a bottle
     */
    Bottle get_partOfSpeech();

  

    /**
     * Get the lemma of the speech analysis for each word of the text.
     * @return a bottle
     */
    Bottle get_lemma();


    /**
     * Get the morphology of the speech analysis for each word of the text.
     * @return a bottle
     */
    Bottle get_morphology();


    /**
     * Get the sentiment of the speech analysis for the whole sentence 
     * @return a bottle
     */
    Bottle get_sentiment();


}
