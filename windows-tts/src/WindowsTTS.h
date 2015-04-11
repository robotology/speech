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
using namespace yarp::sig;

class SpeechRecognizerModule: public RFModule
{
    //Yarp related
    Port m_portRPC;
    Port m_portContinuousRecognition;
    Port m_portContinuousRecognitionGrammar;
    Port m_port2iSpeak;
    Port m_port2iSpeakRpc;
    BufferedPort<Sound> m_portSound;

    //SAPI related
    CComPtr<ISpAudio>           m_cpAudio;    
    CComPtr<ISpRecognizer>      m_cpRecoEngine;
    CComPtr<ISpRecoContext>     m_cpRecoCtxt;
    CComPtr<ISpRecoGrammar>     m_cpGrammarFromFile;  
    CComPtr<ISpRecoGrammar>     m_cpGrammarRuntime; 
    CComPtr<ISpRecoGrammar>     m_cpGrammarDictation;   
    BOOL                        m_bInSound;
    BOOL                        m_bGotReco;
    CSpStreamFormat             m_cAudioFmt;
    CComPtr <ISpStream>         m_streamFormat;
	CComPtr<
    //My Grammar management related
    int m_timeout; //ms
    map<string, list<string> >  m_vocabulories;
    bool    m_useTalkBack;
    bool    USE_LEGACY;
    bool    m_forwardSound;
    string  m_tmpFileFolder;

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
    string getFromDictaction(int timeout, LPCWSTR options=NULL);

    /************************************************************************/
    list< pair<string, double> > waitNextRecognitionLEGACY(int timeout);
    
    /************************************************************************/
    Bottle waitNextRecognition(int timeout);

    /************************************************************************/
    void say(string s, bool wait=true);

    /************************************************************************/
    Bottle toBottle(SPPHRASE* pPhrase, const SPPHRASERULE* pRule );

    /************************************************************************/
    bool respond(const Bottle& cmd, Bottle& reply);

    /************************************************************************/
    bool interruptModule()
    {
        std::cout<<"Interrupting ports...";
        m_portRPC.interrupt();
        m_portContinuousRecognition.interrupt();
        m_portContinuousRecognitionGrammar.interrupt();
        m_port2iSpeak.interrupt();
        m_port2iSpeakRpc.interrupt();
        m_portSound.interrupt();
        std::cout<<"ok"<<std::endl;
        return true;
    }

    /************************************************************************/
    bool close()
    {       
        std::cout<<"Closing ports..."; 
        m_portRPC.close();
        m_portContinuousRecognition.close();
        m_portContinuousRecognitionGrammar.close();
        m_port2iSpeak.close();
        m_port2iSpeakRpc.close();
        m_portSound.close();
        std::cout<<"ok"<<std::endl;
        return true;
    }

    /************************************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /************************************************************************/
    bool loadGrammarFromRf(ResourceFinder &RF);

    /************************************************************************/
    yarp::sig::Sound toSound(CComPtr<ISpRecoResult> cpRecoResult);
};

