/*
 * Copyright (C) 2014 iCub Facility
 * Authors: Ali Paikan
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <yarp/os/Thread.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>
#include <yarp/os/LogStream.h>

#include <speech.h>

using namespace yarp::os;
using namespace yarp::dev;

#define PICO_MEM_SIZE   2500000     /* adaptation layer defines */
#define DummyLen        100000000
#define MAX_OUTBUF_SIZE 128         /* string constants */

const char * PICO_VOICE_NAME    = "PicoVoice";

// supported voices Pico does not seperately specify the voice and locale.
const char * picoSupportedLangIso3[]        = { "eng",              "eng",              "deu",              "spa",              "fra",              "ita" };
const char * picoSupportedCountryIso3[]     = { "USA",              "GBR",              "DEU",              "ESP",              "FRA",              "ITA" };
const char * picoSupportedLang[]            = { "en-US",            "en-GB",            "de-DE",            "es-ES",            "fr-FR",            "it-IT" };
const char * picoInternalLang[]             = { "en-US",            "en-GB",            "de-DE",            "es-ES",            "fr-FR",            "it-IT" };
const char * picoInternalTaLingware[]       = { "en-US_ta.bin",     "en-GB_ta.bin",     "de-DE_ta.bin",     "es-ES_ta.bin",     "fr-FR_ta.bin",     "it-IT_ta.bin" };
const char * picoInternalSgLingware[]       = { "en-US_lh0_sg.bin", "en-GB_kh0_sg.bin", "de-DE_gl0_sg.bin", "es-ES_zl0_sg.bin", "fr-FR_nk0_sg.bin", "it-IT_cm0_sg.bin" };
const char * picoInternalUtppLingware[]     = { "en-US_utpp.bin",   "en-GB_utpp.bin",   "de-DE_utpp.bin",   "es-ES_utpp.bin",   "fr-FR_utpp.bin",   "it-IT_utpp.bin" };
const int picoNumSupportedVocs              = 6;


/****************************************************************
 * @brief The yarp::dev::Speech class
 */
Speech::Speech() {
    pcmDevice.clear();
    language = "en-US";
    supportedLangs.push_back("en-US");
    supportedLangs.push_back("en-GB");
    supportedLangs.push_back("es-ES");
    supportedLangs.push_back("fr-FR");
    supportedLangs.push_back("it-IT");
    supportedLangs.push_back("de-DE");
    // picotts
    picoMemArea         = NULL;
    picoSystem          = NULL;
    picoTaResource      = NULL;
    picoSgResource      = NULL;
    picoUtppResource    = NULL;
    picoEngine          = NULL;
    picoTaFileName      = NULL;
    picoSgFileName      = NULL;
    picoUtppFileName    = NULL;
    picoTaResourceName  = NULL;
    picoSgResourceName  = NULL;
    picoUtppResourceName = NULL;
    picoSynthAbort = 0;
}

Speech::~Speech() {
    close();
}


bool Speech::open(yarp::os::Searchable &config)
{
    Speech::config.fromString(config.toString());

    if(config.check("pcm-device"))
        pcmDevice = config.find("pcm-device").asString();
    if(config.check("default-language"))
        if(!setLanguage(config.find("default-language").asString())) {
            yError()<<"Cannot set the default language to"<<config.find("default-language").asString();
            return false;
        }

    setPitch(config.check("pitch",Value(90)).asInt32());
    setSpeed(config.check("speed",Value(105)).asInt32());

    lingwareRF.setDefaultContext(config.check("lingware-context",Value("speech")).asString());
    lingwareRF.configure(0,NULL);

    this->yarp().attachAsServer(rpcPort);

    std::string robot=config.check("robot",Value("icub")).asString();
    std::string portName=std::string("/"+robot+"/speech:rpc");
    if(!rpcPort.open(portName)) {
        yError()<<"Cannot open port "<<portName;
        return false;
    }

    //return PeriodicThread::start();
    return true;
}

bool Speech::close()
{
    yInfo()<<"closing Speech!";
    Thread::stop();
    return true;
}

bool Speech::threadInit() {
    return true;
}

void Speech::threadRelease() {
    rpcPort.close();
}

void Speech::run() {
}


bool Speech::playWav(const std::string& filename) {
    std::string cmd;
#if WIN32
    cmd = "powershell -c (New-Object Media.SoundPlayer ";
    cmd += filename;
    cmd += ").PlaySync()";
#else
    // aplay --device="plughw:1,0" speech.wav
    cmd = "aplay ";
    if(pcmDevice.size())
        cmd += "--device=\""+pcmDevice+"\" ";
    cmd += filename;
#endif
    yInfo()<<cmd;
    int ret = system(cmd.c_str());
    if(ret != 0) {
        yWarning()<<"Cannot play wave file"<<filename;
        return false;
    }
    return true;
}

bool Speech::setLanguage(const std::string& language) {
    if(std::find(supportedLangs.begin(),
                 supportedLangs.end(),
                 language) == supportedLangs.end()) {
        return false;
    }

    Speech::language = language;
    return true;
}

bool Speech::setSpeed(const int16_t speed) {
    Speech::speed = speed;
    return true;
}

bool Speech::setPitch(const int16_t pitch){
    Speech::pitch = pitch;
    return true;
}

std::vector<std::string> Speech::getSupportedLang() {
    return supportedLangs;
}

int16_t Speech::getSpeed() {
    return speed;
}

int16_t Speech::getPitch(){
    return pitch;
}


bool Speech::say(const std::string& text) {
    std::string waveFile = renderSpeech(text);
    if(waveFile.size() == 0)
        return false;
    return playWav(waveFile);
}

bool Speech::play() {
    return false;
}

bool Speech::pause() {
    return false;
}

bool Speech::stop() {
   return false;
}

const std::string Speech::renderSpeech(const std::string &text) {
    //<pitch level='70'><speed level='100'></speed></pitch>"
    char* cmdText = (char*) std::malloc(text.size()+256);
    std::string filename;
#if WIN32
    if (const char* env_tmp = std::getenv("TMP"))
    {
        filename = env_tmp;
        filename += "\\speech.wav";
    }
    else
        filename = "speech.wav";
    _snprintf
#else
    filename = "/tmp/speech.wav";
    snprintf
#endif
    (cmdText,text.size()+255,
    "<pitch level='%d'><speed level='%d'> %s </speed></pitch>",
    pitch, speed, text.c_str());
    /*
    //pico2wave -len-US -w out.wav "hello!"
    std::string cmd = "pico2wave -l" + language + " -w " + filename;
    cmd = cmd + " \"" + text + "\"";
    int ret = system(cmd.c_str());
    if(ret != 0) {
        yWarning()<<"Cannot render the speech!";
        filename.clear();
    }
*/
    const char * lang = language.c_str();
    int langIndex = -1, langIndexTmp = -1;
    size_t bufferSize = 256;


    /* option: --lang */
    for(langIndexTmp =0; langIndexTmp<picoNumSupportedVocs; langIndexTmp++) {
        if(!std::strcmp(picoSupportedLang[langIndexTmp], lang)) {
            langIndex = langIndexTmp;
            break;
        }
    }
    yAssert(langIndex != -1);

    int ret, getstatus;
    pico_Char * inp = NULL;
    pico_Char * local_text = NULL;
    short       outbuf[MAX_OUTBUF_SIZE/2];
    pico_Int16  bytes_sent, bytes_recv, text_remaining, out_data_type;
    pico_Retstring outMessage;

    picoSynthAbort = 0;

    picoMemArea = std::malloc( PICO_MEM_SIZE );
    if((ret = pico_initialize( picoMemArea, PICO_MEM_SIZE, &picoSystem ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot initialize pico (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Load the text analysis Lingware resource file.   */
    picoTaFileName      = (pico_Char *) std::malloc( PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE );
    std::strcpy((char *) picoTaFileName,   lingwareRF.findFileByName(picoInternalTaLingware[langIndex]).c_str());
    if((ret = pico_loadResource( picoSystem, picoTaFileName, &picoTaResource ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot load text analysis resource file (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Load the signal generation Lingware resource file.   */
    picoSgFileName      = (pico_Char *) std::malloc( PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE );
    std::strcpy((char *) picoSgFileName,   lingwareRF.findFileByName(picoInternalSgLingware[langIndex]).c_str());
    if((ret = pico_loadResource( picoSystem, picoSgFileName, &picoSgResource ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot load signal generation Lingware resource file (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Load the utpp Lingware resource file if exists - NOTE: this file is optional
       and is currently not used. Loading is only attempted for future compatibility.
       If this file is not present the loading will still succeed.                      //
    picoUtppFileName      = (pico_Char *) std::malloc( PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE );
    std::strcpy((char *) picoUtppFileName,   PICO_LINGWARE_PATH);
    std::strcat((char *) picoUtppFileName,   (const char *) picoInternalUtppLingware[langIndex]);
    ret = pico_loadResource( picoSystem, picoUtppFileName, &picoUtppResource );
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    printf("pico_loadResource: %i: %s\n", ret, outMessage);
    */

    /* Get the text analysis resource name.     */
    picoTaResourceName  = (pico_Char *) std::malloc( PICO_MAX_RESOURCE_NAME_SIZE );
    if((ret = pico_getResourceName( picoSystem, picoTaResource, (char *) picoTaResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot get the text analysis resource name (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Get the signal generation resource name. */
    picoSgResourceName  = (pico_Char *) std::malloc( PICO_MAX_RESOURCE_NAME_SIZE );
    if((ret = pico_getResourceName( picoSystem, picoSgResource, (char *) picoSgResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot get the signal generation resource name (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }


    /* Create a voice definition.   */
    if((ret = pico_createVoiceDefinition( picoSystem, (const pico_Char *) PICO_VOICE_NAME ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot create voice definition (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Add the text analysis resource to the voice. */
    if((ret = pico_addResourceToVoiceDefinition( picoSystem, (const pico_Char *) PICO_VOICE_NAME, picoTaResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot add the text analysis resource to the voice (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Add the signal generation resource to the voice. */
    if((ret = pico_addResourceToVoiceDefinition( picoSystem, (const pico_Char *) PICO_VOICE_NAME, picoSgResourceName ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot add the signal generation resource to the voice (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    /* Create a new Pico engine. */
    if((ret = pico_newEngine( picoSystem, (const pico_Char *) PICO_VOICE_NAME, &picoEngine ))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        std::fprintf(stderr, "Cannot create a new pico engine (%i): %s\n", ret, outMessage);
        releasePico();
        return ("");
    }

    local_text = (pico_Char *) cmdText;
    text_remaining = std::strlen((const char *) local_text) + 1;

    inp = (pico_Char *) local_text;

    size_t bufused = 0;

    picoos_Common common = (picoos_Common) pico_sysGetCommon(picoSystem);

    picoos_SDFile sdOutFile = NULL;

    picoos_bool done = TRUE;
    if(TRUE != (done = picoos_sdfOpenOut(common, &sdOutFile,
        (picoos_char *) filename.c_str(), SAMPLE_FREQ_16KHZ, PICOOS_ENC_LIN)))
    {
        std::fprintf(stderr, "Cannot open output wave file\n");
        ret = 1;
        releasePico();
        return ("");
    }

    int8_t* buffer = (int8_t*) std::malloc( bufferSize );
    /* synthesis loop   */
    while (text_remaining) {
        /* Feed the text into the engine.   */
        if((ret = pico_putTextUtf8( picoEngine, inp, text_remaining, &bytes_sent ))) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            std::fprintf(stderr, "Cannot put Text (%i): %s\n", ret, outMessage);
            releasePico();
            return ("");
        }

        text_remaining -= bytes_sent;
        inp += bytes_sent;
        do {
            if (picoSynthAbort) {
                releasePico();
                return ("");
            }
            /* Retrieve the samples and add them to the buffer. */
            getstatus = pico_getData( picoEngine, (void *) outbuf,
                      MAX_OUTBUF_SIZE, &bytes_recv, &out_data_type );
            if((getstatus !=PICO_STEP_BUSY) && (getstatus !=PICO_STEP_IDLE)){
                pico_getSystemStatusMessage(picoSystem, getstatus, outMessage);
                std::fprintf(stderr, "Cannot get Data (%i): %s\n", getstatus, outMessage);
                releasePico();
                return ("");
            }
            if (bytes_recv) {
                if ((bufused + bytes_recv) <= bufferSize) {
                    std::memcpy(buffer+bufused, (int8_t *) outbuf, bytes_recv);
                    bufused += bytes_recv;
                } else {
                    done = picoos_sdfPutSamples(
                                        sdOutFile,
                                        bufused / 2,
                                        (picoos_int16*) (buffer));
                    bufused = 0;
                    std::memcpy(buffer, (int8_t *) outbuf, bytes_recv);
                    bufused += bytes_recv;
                }
            }
        } while (PICO_STEP_BUSY == getstatus);
        /* This chunk of synthesis is finished; pass the remaining samples. */
        if (!picoSynthAbort) {
                    done = picoos_sdfPutSamples(
                                        sdOutFile,
                                        bufused / 2,
                                        (picoos_int16*) (buffer));
        }
        picoSynthAbort = 0;
    }

    if(TRUE != (done = picoos_sdfCloseOut(common, &sdOutFile))) {
        std::fprintf(stderr, "Cannot close output wave file\n");
        ret = 1;
        std::free(buffer);
        releasePico();
        return ("");
    }

    std::free(buffer);
    releasePico();
    return filename;
}

void Speech::releasePico() {

    if (picoEngine) {
        pico_disposeEngine( picoSystem, &picoEngine );
        pico_releaseVoiceDefinition( picoSystem, (pico_Char *) PICO_VOICE_NAME );
        picoEngine = NULL;
    }

    if (picoUtppResource) {
        pico_unloadResource( picoSystem, &picoUtppResource );
        picoUtppResource = NULL;
    }

    if (picoSgResource) {
        pico_unloadResource( picoSystem, &picoSgResource );
        picoSgResource = NULL;
    }

    if (picoTaResource) {
        pico_unloadResource( picoSystem, &picoTaResource );
        picoTaResource = NULL;
    }

    if (picoSystem) {
        pico_terminate(&picoSystem);
        picoSystem = NULL;
    }
    if(picoMemArea) {
        std::free(picoMemArea);
        picoMemArea = NULL;
    }
}
