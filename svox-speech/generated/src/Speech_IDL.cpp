// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <Speech_IDL.h>
#include <yarp/os/idl/WireTypes.h>



class Speech_IDL_setLanguage : public yarp::os::Portable {
public:
  std::string language;
  bool _return;
  void init(const std::string& language);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_setSpeed : public yarp::os::Portable {
public:
  int16_t speed;
  bool _return;
  void init(const int16_t speed);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_setPitch : public yarp::os::Portable {
public:
  int16_t pitch;
  bool _return;
  void init(const int16_t pitch);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_getSpeed : public yarp::os::Portable {
public:
  int16_t _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_getPitch : public yarp::os::Portable {
public:
  int16_t _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_getSupportedLang : public yarp::os::Portable {
public:
  std::vector<std::string>  _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_say : public yarp::os::Portable {
public:
  std::string text;
  bool _return;
  void init(const std::string& text);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_play : public yarp::os::Portable {
public:
  bool _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_pause : public yarp::os::Portable {
public:
  bool _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class Speech_IDL_stop : public yarp::os::Portable {
public:
  bool _return;
  void init();
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

bool Speech_IDL_setLanguage::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(2)) return false;
  if (!writer.writeTag("setLanguage",1,1)) return false;
  if (!writer.writeString(language)) return false;
  return true;
}

bool Speech_IDL_setLanguage::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_setLanguage::init(const std::string& language) {
  _return = false;
  this->language = language;
}

bool Speech_IDL_setSpeed::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(2)) return false;
  if (!writer.writeTag("setSpeed",1,1)) return false;
  if (!writer.writeI16(speed)) return false;
  return true;
}

bool Speech_IDL_setSpeed::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_setSpeed::init(const int16_t speed) {
  _return = false;
  this->speed = speed;
}

bool Speech_IDL_setPitch::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(2)) return false;
  if (!writer.writeTag("setPitch",1,1)) return false;
  if (!writer.writeI16(pitch)) return false;
  return true;
}

bool Speech_IDL_setPitch::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_setPitch::init(const int16_t pitch) {
  _return = false;
  this->pitch = pitch;
}

bool Speech_IDL_getSpeed::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("getSpeed",1,1)) return false;
  return true;
}

bool Speech_IDL_getSpeed::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readI16(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_getSpeed::init() {
  _return = 0;
}

bool Speech_IDL_getPitch::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("getPitch",1,1)) return false;
  return true;
}

bool Speech_IDL_getPitch::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readI16(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_getPitch::init() {
  _return = 0;
}

bool Speech_IDL_getSupportedLang::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("getSupportedLang",1,1)) return false;
  return true;
}

bool Speech_IDL_getSupportedLang::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  {
    _return.clear();
    uint32_t _size0;
    yarp::os::idl::WireState _etype3;
    reader.readListBegin(_etype3, _size0);
    _return.resize(_size0);
    uint32_t _i4;
    for (_i4 = 0; _i4 < _size0; ++_i4)
    {
      if (!reader.readString(_return[_i4])) {
        reader.fail();
        return false;
      }
    }
    reader.readListEnd();
  }
  return true;
}

void Speech_IDL_getSupportedLang::init() {
}

bool Speech_IDL_say::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(2)) return false;
  if (!writer.writeTag("say",1,1)) return false;
  if (!writer.writeString(text)) return false;
  return true;
}

bool Speech_IDL_say::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_say::init(const std::string& text) {
  _return = false;
  this->text = text;
}

bool Speech_IDL_play::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("play",1,1)) return false;
  return true;
}

bool Speech_IDL_play::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_play::init() {
  _return = false;
}

bool Speech_IDL_pause::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("pause",1,1)) return false;
  return true;
}

bool Speech_IDL_pause::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_pause::init() {
  _return = false;
}

bool Speech_IDL_stop::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(1)) return false;
  if (!writer.writeTag("stop",1,1)) return false;
  return true;
}

bool Speech_IDL_stop::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void Speech_IDL_stop::init() {
  _return = false;
}

Speech_IDL::Speech_IDL() {
  yarp().setOwner(*this);
}
bool Speech_IDL::setLanguage(const std::string& language) {
  bool _return = false;
  Speech_IDL_setLanguage helper;
  helper.init(language);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::setLanguage(const std::string& language)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::setSpeed(const int16_t speed) {
  bool _return = false;
  Speech_IDL_setSpeed helper;
  helper.init(speed);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::setSpeed(const int16_t speed)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::setPitch(const int16_t pitch) {
  bool _return = false;
  Speech_IDL_setPitch helper;
  helper.init(pitch);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::setPitch(const int16_t pitch)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
int16_t Speech_IDL::getSpeed() {
  int16_t _return = 0;
  Speech_IDL_getSpeed helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","int16_t Speech_IDL::getSpeed()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
int16_t Speech_IDL::getPitch() {
  int16_t _return = 0;
  Speech_IDL_getPitch helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","int16_t Speech_IDL::getPitch()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
std::vector<std::string>  Speech_IDL::getSupportedLang() {
  std::vector<std::string>  _return;
  Speech_IDL_getSupportedLang helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","std::vector<std::string>  Speech_IDL::getSupportedLang()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::say(const std::string& text) {
  bool _return = false;
  Speech_IDL_say helper;
  helper.init(text);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::say(const std::string& text)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::play() {
  bool _return = false;
  Speech_IDL_play helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::play()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::pause() {
  bool _return = false;
  Speech_IDL_pause helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::pause()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool Speech_IDL::stop() {
  bool _return = false;
  Speech_IDL_stop helper;
  helper.init();
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool Speech_IDL::stop()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}

bool Speech_IDL::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  reader.expectAccept();
  if (!reader.readListHeader()) { reader.fail(); return false; }
  yarp::os::ConstString tag = reader.readTag();
  bool direct = (tag=="__direct__");
  if (direct) tag = reader.readTag();
  while (!reader.isError()) {
    // TODO: use quick lookup, this is just a test
    if (tag == "setLanguage") {
      std::string language;
      if (!reader.readString(language)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = setLanguage(language);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "setSpeed") {
      int16_t speed;
      if (!reader.readI16(speed)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = setSpeed(speed);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "setPitch") {
      int16_t pitch;
      if (!reader.readI16(pitch)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = setPitch(pitch);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "getSpeed") {
      int16_t _return;
      _return = getSpeed();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeI16(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "getPitch") {
      int16_t _return;
      _return = getPitch();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeI16(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "getSupportedLang") {
      std::vector<std::string>  _return;
      _return = getSupportedLang();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        {
          if (!writer.writeListBegin(BOTTLE_TAG_STRING, static_cast<uint32_t>(_return.size()))) return false;
          std::vector<std::string> ::iterator _iter5;
          for (_iter5 = _return.begin(); _iter5 != _return.end(); ++_iter5)
          {
            if (!writer.writeString((*_iter5))) return false;
          }
          if (!writer.writeListEnd()) return false;
        }
      }
      reader.accept();
      return true;
    }
    if (tag == "say") {
      std::string text;
      if (!reader.readString(text)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = say(text);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "play") {
      bool _return;
      _return = play();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "pause") {
      bool _return;
      _return = pause();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "stop") {
      bool _return;
      _return = stop();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "help") {
      std::string functionName;
      if (!reader.readString(functionName)) {
        functionName = "--all";
      }
      std::vector<std::string> _return=help(functionName);
      yarp::os::idl::WireWriter writer(reader);
        if (!writer.isNull()) {
          if (!writer.writeListHeader(2)) return false;
          if (!writer.writeTag("many",1, 0)) return false;
          if (!writer.writeListBegin(BOTTLE_TAG_INT, static_cast<uint32_t>(_return.size()))) return false;
          std::vector<std::string> ::iterator _iterHelp;
          for (_iterHelp = _return.begin(); _iterHelp != _return.end(); ++_iterHelp)
          {
            if (!writer.writeString(*_iterHelp)) return false;
           }
          if (!writer.writeListEnd()) return false;
        }
      reader.accept();
      return true;
    }
    if (reader.noMore()) { reader.fail(); return false; }
    yarp::os::ConstString next_tag = reader.readTag();
    if (next_tag=="") break;
    tag = tag + "_" + next_tag;
  }
  return false;
}

std::vector<std::string> Speech_IDL::help(const std::string& functionName) {
  bool showAll=(functionName=="--all");
  std::vector<std::string> helpString;
  if(showAll) {
    helpString.push_back("*** Available commands:");
    helpString.push_back("setLanguage");
    helpString.push_back("setSpeed");
    helpString.push_back("setPitch");
    helpString.push_back("getSpeed");
    helpString.push_back("getPitch");
    helpString.push_back("getSupportedLang");
    helpString.push_back("say");
    helpString.push_back("play");
    helpString.push_back("pause");
    helpString.push_back("stop");
    helpString.push_back("help");
  }
  else {
    if (functionName=="setLanguage") {
      helpString.push_back("bool setLanguage(const std::string& language) ");
      helpString.push_back("set the speech langauge ");
      helpString.push_back("@return true/false on success/failure ");
    }
    if (functionName=="setSpeed") {
      helpString.push_back("bool setSpeed(const int16_t speed) ");
      helpString.push_back("set the speech speed ");
      helpString.push_back("@return true/false on success/failure ");
    }
    if (functionName=="setPitch") {
      helpString.push_back("bool setPitch(const int16_t pitch) ");
      helpString.push_back("set the speech pitch ");
      helpString.push_back("@return true/false on success/failure ");
    }
    if (functionName=="getSpeed") {
      helpString.push_back("int16_t getSpeed() ");
      helpString.push_back("get the speech speed ");
      helpString.push_back("@return the speed ");
    }
    if (functionName=="getPitch") {
      helpString.push_back("int16_t getPitch() ");
      helpString.push_back("get the speech pitch ");
      helpString.push_back("@return the pitch ");
    }
    if (functionName=="getSupportedLang") {
      helpString.push_back("std::vector<std::string>  getSupportedLang() ");
      helpString.push_back("get the available languages ");
      helpString.push_back("@return a list of the supported language ");
    }
    if (functionName=="say") {
      helpString.push_back("bool say(const std::string& text) ");
      helpString.push_back("render and play the speech ");
      helpString.push_back("@return true/false on success/failure ");
    }
    if (functionName=="play") {
      helpString.push_back("bool play() ");
      helpString.push_back("play the previously rendered or paused speech ");
    }
    if (functionName=="pause") {
      helpString.push_back("bool pause() ");
      helpString.push_back("pause the current speech ");
    }
    if (functionName=="stop") {
      helpString.push_back("bool stop() ");
      helpString.push_back("stop playing the current speech ");
    }
    if (functionName=="help") {
      helpString.push_back("std::vector<std::string> help(const std::string& functionName=\"--all\")");
      helpString.push_back("Return list of available commands, or help message for a specific function");
      helpString.push_back("@param functionName name of command for which to get a detailed description. If none or '--all' is provided, print list of available commands");
      helpString.push_back("@return list of strings (one string per line)");
    }
  }
  if ( helpString.empty()) helpString.push_back("Command not found");
  return helpString;
}


