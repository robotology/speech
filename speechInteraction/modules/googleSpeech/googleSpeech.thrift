# Copyright (C) 2019 Fondazione Istituto Italiano di Tecnologia (IIT)  
# All Rights Reserved.
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
#
# googleSpeech.thrift

/**
* googleSpeech_IDL
*
* IDL Interface to google speech cloud services 
*/

service googleSpeech_IDL
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
     * Get the state of the google service.
     * @return a string containing the state
     */
    string getState();
    
     /**
     * Get the time needed for the google service to process the request.
     * @return a double containing the processing time
     */
    i64 getProcessingTime();

      /**
     * Get a feedback from the google service to know if an error occured.
     * @return a string containing the feedback
     */
    string getFeedback();
}
