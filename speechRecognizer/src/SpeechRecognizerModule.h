/* 
 * Copyright (C) 2011 EFAA Consortium, European Commission FP7 Project IST-270490
 * Authors: Stephane Lallee
 * email:   stephane.lallee@gmail.com
 * website: http://efaa.upf.edu/ 
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include "stdafx.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_EDIT_TEXT   1000
#define GID_DICTATION   0           // Dictation grammar has grammar ID 0

#define WM_RECOEVENT    WM_APP      // Window message used for recognition events
using namespace std;
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
    BOOL                        m_bInSound;
    BOOL                        m_bGotReco;

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
    string getFromDictaction(int timeout);

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

