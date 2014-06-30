// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_Speech_IDL
#define YARP_THRIFT_GENERATOR_Speech_IDL

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>

class Speech_IDL;


/**
 * Speech_IDL
 */
class Speech_IDL : public yarp::os::Wire {
public:
  Speech_IDL();
  /**
   * set the speech langauge
   * @return true/false on success/failure
   */
  virtual bool setLanguage(const std::string& language);
  /**
   * set the speech speed
   * @return true/false on success/failure
   */
  virtual bool setSpeed(const int16_t speed);
  /**
   * set the speech pitch
   * @return true/false on success/failure
   */
  virtual bool setPitch(const int16_t pitch);
  /**
   * get the speech speed
   * @return the speed
   */
  virtual int16_t getSpeed();
  /**
   * get the speech pitch
   * @return the pitch
   */
  virtual int16_t getPitch();
  /**
   * get the available languages
   * @return a list of the supported language
   */
  virtual std::vector<std::string>  getSupportedLang();
  /**
   * render and play the speech
   * @return true/false on success/failure
   */
  virtual bool say(const std::string& text);
  /**
   * play the previously rendered or paused speech
   */
  virtual bool play();
  /**
   * pause the current speech
   */
  virtual bool pause();
  /**
   * stop playing the current speech
   */
  virtual bool stop();
  virtual bool read(yarp::os::ConnectionReader& connection);
  virtual std::vector<std::string> help(const std::string& functionName="--all");
};

#endif
