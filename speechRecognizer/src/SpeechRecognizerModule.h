// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright © Microsoft Corporation. All rights reserved

/****************************************************************************
*   SimpleDictation.h
*       Definition of the SpeechRecognizerModule class
*****************************************************************************/
#include "stdafx.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_EDIT_TEXT   1000
#define GID_DICTATION   0           // Dictation grammar has grammar ID 0

#define WM_RECOEVENT    WM_APP      // Window message used for recognition events
using namespace std;
using namespace yarp;
using namespace yarp::os;

class SpeechRecognizerModule: public RFModule
{
    //Yarp related
    Port m_portRPC;
    Port m_portContinuousRecognition;
    Port m_portContinuousRecognitionGrammar;
    Port m_port2iSpeak;

    //SAPI related
    CComPtr<ISpAudio>           m_cpAudio;    
    CComPtr<ISpRecognizer>      m_cpRecoEngine;
    CComPtr<ISpRecoContext>     m_cpRecoCtxt;
    CComPtr<ISpRecoGrammar>     m_cpGrammarFromFile;  
    CComPtr<ISpRecoGrammar>     m_cpGrammarRuntime; 
    CComPtr<ISpRecoGrammar>     m_cpGrammarDictation;

    //My Grammar management related
    int m_timeout; //ms
    map<string, list<string> >  m_vocabulories;
    bool    m_useTalkBack;
    bool    USE_LEGACY;

public:

    /************************************************************************/
    bool configure(ResourceFinder &rf);

    /************************************************************************/
    bool updateModule();

    /************************************************************************/
    bool handleRGMCmd(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool handleAsyncRecognitionCmd(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool refreshFromVocabulories(CComPtr<ISpRecoGrammar> grammarToModify);

    /************************************************************************/
    bool handleVocabuloryCmd(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool handleRecognitionCmd(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool setGrammarCustom(CComPtr<ISpRecoGrammar> grammarToModify, string grammar, bool append);

    /************************************************************************/
    list< pair<string, double> > waitNextRecognitionLEGACY(int timeout);
    
    /************************************************************************/
    Bottle waitNextRecognition(int timeout);

    /************************************************************************/
    void say(string s);

    /************************************************************************/
    Bottle toBottle(SPPHRASE* pPhrase, const SPPHRASERULE* pRule );

    /************************************************************************/
    bool respond(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool interruptModule()
    {
        return true;
    }

    /************************************************************************/
    bool close()
    {
        return true;
    }

    /************************************************************************/
    double getPeriod()
    {
        return 0.5;
    }

    /************************************************************************/
    bool loadGrammarFromRf(ResourceFinder &RF);


};

