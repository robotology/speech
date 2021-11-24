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

#include "SpeechRecognizerModule.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Helpers for dealing with the weird strings of windows... 
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}
std::string ws2s(LPCWSTR s)
{
    char    *pmbbuf   = (char *)malloc( 100 );
    wcstombs( pmbbuf, s, 100 );
    return pmbbuf;
}
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Module implementation
bool SpeechRecognizerModule::configure(ResourceFinder &rf )
{
    setName( rf.check("name",Value("speechRecognizer")).asString().c_str() );
    m_timeout = rf.check("timeout",Value(10000)).asInt32();
    USE_LEGACY = !rf.check("noLegacy");
    m_forwardSound = rf.check("forwardSound");
    m_tmpFileFolder = rf.getHomeContextPath();
    interruptRecognition = false;

    //Deal with speech recognition
    string grammarFile = rf.check("grammarFile",Value("defaultGrammar.grxml")).asString();
    grammarFile = rf.findFile(grammarFile);

    std::wstring tmp = s2ws(grammarFile);
    LPCWSTR cwgrammarfile = tmp.c_str();

    m_useTalkBack = rf.check("talkback");

    //Initialise the speech crap
    bool everythingIsFine = true;
    HRESULT hr;
    everythingIsFine = SUCCEEDED( m_cpRecoEngine.CoCreateInstance(CLSID_SpInprocRecognizer));
    everythingIsFine &= SUCCEEDED( SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &m_cpAudio));
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine->CreateRecoContext( &m_cpRecoCtxt ));

    // Here, all we are interested in is the beginning and ends of sounds, as well as
    // when the engine has recognized something
    const ULONGLONG ullInterest = SPFEI(SPEI_RECOGNITION);
    everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->SetInterest(ullInterest, ullInterest));

    // set the input for the engine
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine->SetInput(m_cpAudio, TRUE));
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine->SetRecoState( SPRST_ACTIVE ));

    //Load grammar from file
    everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 1, &m_cpGrammarFromFile ));
    everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_DISABLED));
    everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));
//  everythingIsFine &= loadGrammarFromRf(rf);

    //Create a runtime grammar
    everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 2, &m_cpGrammarRuntime ));
    everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));

    //Create a dictation grammar
    everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->CreateGrammar( GID_DICTATION, &m_cpGrammarDictation ));
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->LoadDictation(NULL, SPLO_STATIC));
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState(SPRS_INACTIVE));
    
    //Setup thing for the raw audio processing
    everythingIsFine &= SUCCEEDED(m_cAudioFmt.AssignFormat(SPSF_22kHz16BitMono));
    hr = m_cpRecoCtxt->SetAudioOptions(SPAO_RETAIN_AUDIO, &m_cAudioFmt.FormatId(), m_cAudioFmt.WaveFormatExPtr());
    //everythingIsFine &= SUCCEEDED(hr = SPBindToFile((const WCHAR *)"C:\\temp.wav", SPFM_CREATE_ALWAYS, &m_streamFormat, &m_cAudioFmt.FormatId(), m_cAudioFmt.WaveFormatExPtr()));

    //CComPtr <ISpStream>     cpStream = NULL;
    //CSpStreamFormat         cAudioFmt;
    //hr = cAudioFmt.AssignFormat(SPSF_22kHz16BitMono);
    //hr = SPBindToFile((const WCHAR *)"c:\\ttstemp.wav", SPFM_CREATE_ALWAYS, &cpStream,  &cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr());

    if( everythingIsFine )
    {
        string pName = "/";
        pName += getName();
        pName += "/recog/continuous:o";
        m_portContinuousRecognition.open( pName );

        pName = "/";
        pName += getName();
        pName += "/recog/continuousGrammar:o";
        m_portContinuousRecognitionGrammar.open( pName );

        pName = "/";
        pName += getName();
        pName += "/recog/sound:o";   
        m_portSound.open( pName );

        //iSpeak
        pName = "/";
        pName += getName();
        pName += "/tts/iSpeak:o"; 
        m_port2iSpeak.open( pName );
                
        pName = "/";
        pName += getName();
        pName += "/tts/iSpeak/rpc"; 
        m_port2iSpeakRpc.open( pName );
        if (Network::connect(m_port2iSpeak.getName(),"/iSpeak")&&Network::connect(m_port2iSpeakRpc.getName(),"/iSpeak/rpc"))
            yInfo() <<"Connection to iSpeak succesfull" ;
        else
            yWarning() <<"Unable to connect to iSpeak. Connect manually." ;

        pName = "/";
        pName += getName();
        pName += "/rpc";
        m_portRPC.open( pName );
        attach(m_portRPC);

        //Start recognition
        //everythingIsFine &= SUCCEEDED(m_cpRecoEngine->SetRecoState(SPRST_ACTIVE_ALWAYS));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));
    }

    return (everythingIsFine);
}
yarp::sig::Sound SpeechRecognizerModule::toSound(CComPtr<ISpRecoResult> cpRecoResult)
{
    HRESULT hr = S_OK;
    CComPtr<ISpStreamFormat>      cpStreamFormat = NULL;
    SPPHRASE* pPhrase;
    bool successGetPhrase = SUCCEEDED(cpRecoResult->GetPhrase(&pPhrase));
    hr = cpRecoResult->GetAudio(0, pPhrase->Rule.ulCountOfElements, &cpStreamFormat);

    CComPtr<ISpStream> cpStream;
    ULONG cbWritten = 0; 
   
    string sPath = m_tmpFileFolder + "//tmp.wav";
    //static const WCHAR path[] = L"C://tmpSnd.wav";

    // create file on hard-disk for storing recognized audio, and specify audio format as the retained audio format
    hr = SPBindToFile(s2ws(sPath).c_str(), SPFM_CREATE_ALWAYS, &cpStream, &m_cAudioFmt.FormatId(), m_cAudioFmt.WaveFormatExPtr(), SPFEI_ALL_EVENTS);

    //Continuously transfer data between the two streams until no more data is found (i.e. end of stream)
    //Note only transfer 1000 bytes at a time to creating large chunks of data at one time
    while (TRUE)
    {
        // for logging purposes, the app can retrieve the recognized audio stream length in bytes
        STATSTG stats;
        hr = cpStreamFormat->Stat(&stats, NULL);
        // Check hr

        // create a 1000-byte buffer for transferring
        BYTE bBuffer[1000];
        ULONG cbRead;

        // request 1000 bytes of data from the input stream
        hr = cpStreamFormat->Read(bBuffer, 1000, &cbRead);
        // if data was returned�
        if (SUCCEEDED(hr) && cbRead > 0)
        {
            //then transfer/write the audio to the file-based stream
                hr = cpStream->Write(bBuffer, cbRead, &cbWritten);
            // Check hr
        }

        // since there is no more data being added to the input stream, if the read request returned less than expected, the end of stream was reached, so break data transfer loop
        if (cbRead < 1000)
        {
            break;
        }
    }
    cpStream->Close();
    cpStream.Release();

    yarp::sig::Sound s;
    yarp::sig::file::read(s, sPath.c_str());
    return s;
    return true;
}


/************************************************************************/
bool SpeechRecognizerModule::updateModule()
{
    cout<<".";
    USES_CONVERSION;
    CSpEvent event;

    // Process all of the recognition events
    while (event.GetFrom(m_cpRecoCtxt) == S_OK)
    {
        switch (event.eEventId)
        {
            case SPEI_SOUND_START:
                {
                    m_bInSound = TRUE;
                    yInfo() << "Sound in...";
                    break;
                }

            case SPEI_SOUND_END:
                if (m_bInSound)
                {
                    m_bInSound = FALSE;
                    if (!m_bGotReco)
                    {
                        // The sound has started and ended, 
                        // but the engine has not succeeded in recognizing anything
                        yWarning() << "Chunk of sound detected: Recognition is null";
                    }
                    m_bGotReco = FALSE;
                }
                break;

            case SPEI_RECOGNITION:
                // There may be multiple recognition results, so get all of them
                {
                    m_bGotReco = TRUE;
                    static const WCHAR wszUnrecognized[] = L"<Unrecognized>";

                    CSpDynamicString dstrText;
                    if (SUCCEEDED(event.RecoResult()->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                                                            &dstrText, NULL)))
                    {
                        SPPHRASE* pPhrase = NULL;
                        bool successGetPhrase = SUCCEEDED(event.RecoResult()->GetPhrase(&pPhrase));  
                        int confidence=pPhrase->Rule.Confidence;
                        
                        string fullSentence = ws2s(dstrText);
                        yInfo() <<"Recognized "<<fullSentence<<" with confidence "<<confidence ;
                        

                        //Send over yarp
                        Bottle bOut;
                        bOut.addString(fullSentence);
                        bOut.addInt32(confidence);
                        m_portContinuousRecognition.write(bOut);

                        //Treat the semantic
                        if (successGetPhrase)
                        {
                            //Send sound
                            if (m_forwardSound)
                            {
                                yarp::sig::Sound& rawSnd = m_portSound.prepare();
                                rawSnd = toSound(event.RecoResult());
                                m_portSound.write();
                            }

                            //--------------------------------------------------- 1. 1st subBottle : raw Sentence -----------------------------------------------//
                            int wordCount = pPhrase->Rule.ulCountOfElements;
                            string rawPhrase = "";
                            for(int i=0; i< wordCount; i++){
                                rawPhrase += ws2s(pPhrase->pElements[i].pszDisplayText) + " ";
                                yDebug() << "word : " <<  ws2s(pPhrase->pElements[i].pszDisplayText) ;
                            }
                            yInfo() <<"Raw sentence: "<<rawPhrase ;
                            if (&pPhrase->Rule == NULL)
                            {
                                yError() <<"Cannot parse the sentence!";
                                return true;
                            }
                            //--------------------------------------------------- 2. 2nd subottle : Word/Role ---------------------------------------------------//
                            Bottle bOutGrammar;
                            bOutGrammar.addString(rawPhrase);
                            bOutGrammar.addList()=toBottle(pPhrase,&pPhrase->Rule);
                            yInfo() << "Sending semantic bottle : " << bOutGrammar.toString();
                            m_portContinuousRecognitionGrammar.write(bOutGrammar);
                            ::CoTaskMemFree(pPhrase);
                        }

                        if (m_useTalkBack)
                            say(fullSentence);
                    }
                }
                break;
        }
    }
    return true;
}

/************************************************************************/
Bottle SpeechRecognizerModule::toBottle(SPPHRASE* pPhrase, const SPPHRASERULE* pRule)
{
    Bottle bCurrentLevelGlobal;

    const SPPHRASERULE* siblingRule = pRule;
    while (siblingRule != NULL)
    {   
        Bottle bCurrentSubLevel;
        bCurrentSubLevel.addString(ws2s(siblingRule->pszName));

        //we backtrack
        if(siblingRule->pFirstChild != NULL )
        {
            bCurrentSubLevel.addList()=toBottle(pPhrase, siblingRule->pFirstChild);
        }
        else
        {
            string nodeString = "";
            for(unsigned int i=0; i<siblingRule->ulCountOfElements; i++)
            {
                nodeString += ws2s(pPhrase->pElements[siblingRule->ulFirstElement + i].pszDisplayText);
                if (i<siblingRule->ulCountOfElements-1)
                    nodeString += " ";
            }
            bCurrentSubLevel.addString(nodeString);
        }
        siblingRule = siblingRule->pNextSibling;
        if (pRule->pNextSibling !=NULL)
            bCurrentLevelGlobal.addList() = bCurrentSubLevel;
        else
            bCurrentLevelGlobal = bCurrentSubLevel;

    }
    return bCurrentLevelGlobal;
}

/************************************************************************/
bool SpeechRecognizerModule::respond(const Bottle& cmd, Bottle& reply) 
{
    reply.addString("ACK");
    string firstVocab = cmd.get(0).asString();

    if (firstVocab == "tts")
    {
        string sentence = cmd.get(1).asString();
        say(sentence);
        reply.addString("OK");
    }
    else if (firstVocab == "RGM" || firstVocab == "rgm" )
    {    
        string secondVocab = cmd.get(1).asString();
        if (secondVocab=="vocabulory")
            handleRGMCmd(cmd.tail().tail(), reply);
    }
    else if (firstVocab == "recog")
    {
        handleRecognitionCmd(cmd.tail(), reply);
    }
    else if (firstVocab == "asyncrecog")
    {
        handleAsyncRecognitionCmd(cmd.tail(), reply);
    }
    else if (firstVocab == "interrupt")
    {
        handleInterrupt(cmd.tail(), reply);
    }
    else
        reply.addString("UNKNOWN");

    return true;
}

/************************************************************************/
bool SpeechRecognizerModule::handleInterrupt(const Bottle& cmd, Bottle& reply)
{
    yInfo() << "Grammar interrupted";
    interruptRecognition = true;
    yarp::os::Time::delay(0.5);
    interruptRecognition = true; // just in case of a previous race condition
    reply.addString("OK");
    return true;
}

/************************************************************************/
bool SpeechRecognizerModule::handleRGMCmd(const Bottle& cmd, Bottle& reply)
{
    string firstVocab = cmd.get(0).asString();
    if (firstVocab == "add")
    {
        string vocabulory = cmd.get(1).asString();
        if (vocabulory[0] != '#')
        {
            //reply.addString("Vocabulories have to start with a #. #Dictation and #WildCard are reserved. Aborting.");
            reply.addString("ERROR");
            return true;
        }
        string word = cmd.get(2).asString();
        m_vocabulories[vocabulory].push_back(word);
        refreshFromVocabulories(m_cpGrammarFromFile);
        reply.addString("OK");
        return true;
    }

    if (firstVocab == "addAuto")
    {
        string vocabuloryType = cmd.get(1).asString();
        yInfo() <<"Trying to enrich the "<<vocabuloryType<<" vocabulary.";

        say("Let's improve my dictionary.");
        
        //Try first with open dictation
        int TRIALS_BEFORE_SPELLING = 2;
        bool isFine = false;
        int trial=0;
        string newWord = "";
        while(!isFine && trial<TRIALS_BEFORE_SPELLING)
        {
            say("Please, say the word.");
            newWord = "";
            while(newWord=="")
                newWord=getFromDictaction(m_timeout);
            say("I understood "+ newWord + ". Did you say that?");
                    
            Bottle cmdTmp, replyTmp;
            cmdTmp.addString("grammarSimple");
            cmdTmp.addString("Yes I did.|No I did not.|Skip");
            bool gotAConfirmation = false;
            while(!gotAConfirmation)
            {
                replyTmp.clear();
                handleRecognitionCmd(cmdTmp,replyTmp);
                //cout<<"DEBUG="<<replyTmp.toString()<<endl;
                //cout<<"DEBUG FIRST ELEMENT =|"<<replyTmp.get(0).asString()<<"|"<<endl;
                if (replyTmp.get(0).asString() == "Skip")
                {
                    say("Fine, we give up.");
                    reply.addString("ERROR");
                    return true;
                }
                gotAConfirmation = 
                    replyTmp.size()>0 &&
                    (replyTmp.get(0).asString() == "Yes" ||
                    replyTmp.get(0).asString() == "No");
            }
            //cout<<"Reply is "<<replyTmp.toString()<<endl;
            if ( replyTmp.get(0).asString() == "Yes")
                isFine = true;
            else
                trial++;
        }

        //Try then with spelling
        int TRIALS_BEFORE_GIVING_UP = 2;
        trial =0;
        while(!isFine && trial<TRIALS_BEFORE_GIVING_UP)
        {
            say("Sorry, I cannot get it. Please, spell this word for me?");
            string spelledWord =  "";
            while(spelledWord == "")
                spelledWord=getFromDictaction(m_timeout,SPTOPIC_SPELLING);
            newWord = spelledWord;

            say("I understood "+ newWord + ". Is that right?");
            
            Bottle cmdTmp, replyTmp;
            cmdTmp.addString("grammarSimple");
            cmdTmp.addString("Yes I did.|No I did not.|Skip");
            bool gotAConfirmation = false;
            while(!gotAConfirmation)
            {
                replyTmp.clear();
                handleRecognitionCmd(cmdTmp,replyTmp);
                //cout<<"DEBUG="<<replyTmp.toString()<<endl;
                //cout<<"DEBUG FIRST ELEMENT =|"<<replyTmp.get(0).asString()<<"|"<<endl;
                if (replyTmp.get(0).asString() == "Skip")
                {
                    say("Fine, we give up.");
                    reply.addString("ERROR");
                    return true;
                }

                gotAConfirmation = 
                    replyTmp.size()>0 &&
                    (replyTmp.get(0).asString() == "Yes" ||
                    replyTmp.get(0).asString() == "No");
            }
            //cout<<"Reply is "<<replyTmp.toString()<<endl;
            if ( replyTmp.get(0).asString() == "Yes")
                isFine = true;
            else
                trial++;
        }
        //Give up
        if (!isFine)
        {
            say("Sorry, I think we should give up with this word.");
            reply.addString("ERROR");
        }
        else
        {
            say("Perfect! I know the word " + newWord);
            m_vocabulories[vocabuloryType].push_back(newWord);
            refreshFromVocabulories(m_cpGrammarFromFile);
            reply.addString(newWord);
        }
        
        return true;
    }
    reply.addString("UNKNOWN");
    return false;
}

/************************************************************************/
bool SpeechRecognizerModule::handleAsyncRecognitionCmd(const Bottle& cmd, Bottle& reply)
{
    HRESULT hr;
    string firstVocab = cmd.get(0).asString();
    if (firstVocab == "getGrammar")
    {
        reply.addString("NOT_IMPLEMENTED");
        return true;
    }

    if (firstVocab == "clear")
    {
        bool everythingIsFine=true;
        SPSTATEHANDLE rootRule;
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_DISABLED));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->GetRule(L"rootRule", NULL, SPRAF_TopLevel | SPRAF_Active, TRUE, &rootRule));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->ClearRule(rootRule));
        everythingIsFine &= SUCCEEDED(hr = m_cpGrammarFromFile->Commit(NULL));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));        
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(0));
        reply.addString("Cleared");
        return true;
    }

    if (firstVocab == "addGrammar")
    {    
        string grammar = cmd.get(1).asString();
        bool everythingIsFine = setGrammarCustom(m_cpGrammarFromFile,grammar,true);
        reply.addString("Added");
        return true;
    }

    if (firstVocab == "loadXML")
    { 
        string xml = cmd.get(1).asString();
        ofstream fileTmp("grammarTmp.grxml");
        fileTmp<<xml;
        fileTmp.close();

        std::wstring tmp = s2ws("grammarTmp.grxml");
        LPCWSTR cwgrammarfile = tmp.c_str();

        bool everythingIsFine =true;
        //everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 1, &m_cpGrammarFromFile ));
        everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_DISABLED));
        everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));

        everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(0));

        refreshFromVocabulories(m_cpGrammarFromFile);
        reply.addString("Loaded");
        return true;
    }

    return false;
}

/************************************************************************/
bool SpeechRecognizerModule::refreshFromVocabulories(CComPtr<ISpRecoGrammar> grammarToModify)
{
    //return true;
    bool everythingIsFine = true;

    everythingIsFine &= SUCCEEDED(grammarToModify->SetGrammarState(SPGS_DISABLED)); 

    //Build a rule for each vocabulory
    for(map<string, list<string> >::iterator vIt = m_vocabulories.begin(); vIt != m_vocabulories.end(); vIt++)
    {
        //Get the rule name from the key in the dictionary (i.e Agent, Action, etc...)
        string removedSharp =vIt->first;
        removedSharp.erase(0,1);
        std::wstring tmp = s2ws(removedSharp);
        LPCWSTR cwRuleName = tmp.c_str();

        SPSTATEHANDLE hinit,hstate;
        HRESULT hr;
        //Get the rule or create it
        everythingIsFine &= SUCCEEDED(hr = grammarToModify->GetRule(cwRuleName, NULL, SPRAF_Dynamic, false, &hinit));
        everythingIsFine &= SUCCEEDED(hr = grammarToModify->ClearRule(hinit));
        for(list<string>::iterator wordIt = vIt->second.begin() ; wordIt != vIt->second.end(); wordIt++)
        {
            std::wstring wordTmp = s2ws(*wordIt);
            LPCWSTR cwWord = wordTmp.c_str();
            everythingIsFine &= SUCCEEDED( grammarToModify->AddWordTransition(hinit, NULL, cwWord, NULL, SPWT_LEXICAL, 1, NULL) );
        }
    }

    everythingIsFine &= SUCCEEDED(grammarToModify->Commit(NULL));
    everythingIsFine &= SUCCEEDED(grammarToModify->SetGrammarState(SPGS_ENABLED)); 
    everythingIsFine &= SUCCEEDED(grammarToModify->SetRuleState(NULL, NULL, SPRS_ACTIVE));
    yInfo() << "Grammar is paused, DO NOT SPEAK! (if next message is coming after too long, check your microphone level and lower it!" ;
    everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(NULL));
    yInfo() << "Grammar is resumed : everything is fine = " << everythingIsFine; 

    return everythingIsFine;
}
    
/************************************************************************/
string SpeechRecognizerModule::getFromDictaction(int timeout, LPCWSTR options )
{
    bool everythingIsFine = TRUE;
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->UnloadDictation());
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->LoadDictation(options, SPLO_STATIC));
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState( SPRS_ACTIVE ));
    yInfo() <<"Dictation is on..." ;
    Bottle botTmp;
    if (!USE_LEGACY)
    {
        botTmp = waitNextRecognition(m_timeout);
    }
    else
    {   
        list< pair<string, double> > results = waitNextRecognitionLEGACY(m_timeout);
        for(list< pair<string, double> >::iterator it = results.begin(); it != results.end(); it++)
        {
            botTmp.addString(it->first);
            //botTmp.addFloat64(it->second);
        }
    }
    yInfo() <<"Dictation is off...";
    yInfo() <<"Got : "<<botTmp.toString();
    //Turn off dictation and go back to the file grammar
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState( SPRS_INACTIVE ));
    everythingIsFine &=SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));
    return botTmp.toString();
}

/************************************************************************/
bool SpeechRecognizerModule::handleRecognitionCmd(const Bottle& cmd, Bottle& reply)
{
    string firstVocab = cmd.get(0).asString();

    if (firstVocab == "timeout")
    {
        m_timeout = cmd.get(1).asInt32();
        //reply.addInt32(true);
        return false;
    }

    else if (firstVocab == "dictation")
    {
        bool everythingIsFine = TRUE;
        everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState( SPRS_ACTIVE ));
        yInfo() <<"Dictation is on..." ;

        if (!USE_LEGACY)
        {
            reply.addList() = waitNextRecognition(m_timeout);
        }
        else
        {   
            list< pair<string, double> > results = waitNextRecognitionLEGACY(m_timeout);
            if (results.size()>0)
                for(list< pair<string, double> >::iterator it = results.begin(); it != results.end(); it++)
                {
                    reply.addString(it->first);
                    reply.addFloat64(it->second);
                }
            else
                reply.addString("-1");
        }
        yInfo() <<"Dictation is off...";

        //Turn off dictation and go back to the file grammar
        everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState( SPRS_INACTIVE ));
        everythingIsFine &=SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));  
        //reply.addInt32(true);
        return true;
    }    
    // If we are not in dictation then we set and switch to the runtimeGrammar
    else if (firstVocab == "grammarXML")
    {
        string xml = cmd.get(1).asString();
        ofstream fileTmp("grammarTmp.grxml");
        fileTmp<<xml;
        fileTmp.close();

        std::wstring tmp = s2ws("grammarTmp.grxml");
        LPCWSTR cwgrammarfile = tmp.c_str();

        bool everythingIsFine =true;
        everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));
        everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));
        everythingIsFine &= SUCCEEDED(m_cpGrammarRuntime->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(0));

        refreshFromVocabulories(m_cpGrammarRuntime);

        //reply.addInt32(everythingIsFine);
    }

    else if (firstVocab == "choices")
    {
        string choices ="";
        for (int wI = 1; wI < cmd.size(); wI++)
        {
            choices+=cmd.get(wI).asString();
            if (wI<cmd.size()-1)
                choices+="|";
        }
        setGrammarCustom(m_cpGrammarRuntime,choices,false);
    }
    else if (firstVocab == "grammarSimple")
    {
        string RADStyle = cmd.get(1).asString();
        yInfo() <<"Setting runtime grammar to : "<<RADStyle ;
        setGrammarCustom(m_cpGrammarRuntime,RADStyle,false);
    }
    else 
    {
        reply.addString("UNKNOWN");
        return false;
    }

    //Disable the from file grammar
    SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_DISABLED)); 
    SUCCEEDED(m_cpGrammarRuntime->SetGrammarState(SPGS_ENABLED));         

    //Force blocking recognition
    if (!USE_LEGACY)
    {
        reply.addList() = waitNextRecognition(m_timeout);
    }
    else
    {   
        list< pair<string, double> > results = waitNextRecognitionLEGACY(m_timeout);
        if (results.size()>0)
            for(list< pair<string, double> >::iterator it = results.begin(); it != results.end(); it++)
            {
                reply.addString(it->first);
                reply.addFloat64(it->second);
            }
        else
            reply.addString("-1");
    }
    //Disable the runtime grammar
    SUCCEEDED(m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));   
    SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));        
    return true;
}
/************************************************************************/
Bottle SpeechRecognizerModule::waitNextRecognition(int timeout)
{
    yInfo() <<"Recognition: blocking mode on" ;
    Bottle bOutGrammar;

    bool gotSomething = false;
    double endTime = Time::now() + timeout/1000.0;
    interruptRecognition = false;

    cout << endl ;
    yInfo() << "=========== GO Waiting for recog! ===========" ;

    while(Time::now()<endTime && !gotSomething && !interruptRecognition)
    {
        //std::cout<<".";
        const float ConfidenceThreshold = 0.3f;
        SPEVENT curEvent;
        ULONG fetched = 0;
        HRESULT hr = S_OK;

        m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);

        while (fetched > 0)
        {                   
            yInfo() << " received something in waitNextRecognition" ;
            gotSomething = true;            
            ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);
            CSpDynamicString dstrText;
            result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
            string fullSentence = ws2s(dstrText);
            yInfo() <<fullSentence ;
            if (m_useTalkBack)
                say(fullSentence);
            bOutGrammar.addString(fullSentence);

            SPPHRASE* pPhrase = NULL;
            result->GetPhrase(&pPhrase);    
            bOutGrammar.addList() = toBottle(pPhrase,&pPhrase->Rule);
            yInfo() <<"Sending semantic bottle : "<<bOutGrammar.toString() ;
            m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);

            if (m_forwardSound)
            {
                yarp::sig::Sound& rawSnd = m_portSound.prepare();
                rawSnd = toSound(result);
                m_portSound.write();
            }

        }
    }

    if(interruptRecognition) {
        yDebug() << "interrupted speech recognizer!";
    }
    yInfo() <<"Recognition: blocking mode off";
    return bOutGrammar;
}

/************************************************************************/
list< pair<string, double> > SpeechRecognizerModule::waitNextRecognitionLEGACY(int timeout)
{
    yInfo() <<"Recognition LEGACY: blocking mode on" ;
    list< pair<string, double> > recognitionResults;

    bool gotSomething = false;
    double endTime = Time::now() + timeout/1000.0;
    while(Time::now()<endTime && !gotSomething && !interruptRecognition)
    {
        //std::cout<<".";
        const float ConfidenceThreshold = 0.3f;
        SPEVENT curEvent;
        ULONG fetched = 0;
        HRESULT hr = S_OK;

        m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);

        while (fetched > 0)
        {
            gotSomething = true;
            ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);

            //Convert the catched sentence to strings. 
            CSpDynamicString dstrText;
            result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
            string fullSentence = ws2s(dstrText);
            yInfo() <<fullSentence ;
            
            if (m_useTalkBack)
                say(fullSentence);
            vector<string> words = split(fullSentence,' ');
            for(unsigned int w=0;w<words.size();w++)
            {
                //Todo extract the confidence value somehow...
                recognitionResults.push_back(make_pair(words[w], -1.0));
            }
            m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);
        }
    }
    interruptRecognition = false;
    yInfo() <<"Recognition: blocking mode off" ;
    return recognitionResults;
}

/************************************************************************/
void SpeechRecognizerModule::say(string s, bool wait)
{
    yInfo() <<"TTS: "<<s ;
    Bottle b;
    b.addString(s);
    m_port2iSpeak.write(b);
    if(wait)
    {
        yarp::os::Bottle cmd,reply;
        cmd.addVocab32(VOCAB('s','t','a','t'));
        std::string status = "speaking";
        bool speechStarted = false;
        while(wait&&(!speechStarted ||status=="speaking"))
        {
            m_port2iSpeakRpc.write(cmd,reply);
            status = reply.get(0).asString();
            if (!speechStarted && status != "quiet")
            {
                speechStarted = true;
            }
            yarp::os::Time::delay(0.2);
        }
    }
}

/************************************************************************/
bool  SpeechRecognizerModule::setGrammarCustom(CComPtr<ISpRecoGrammar> grammarToModify, string grammar, bool append)
{
    //Clear the existing runtime grammar
    SPSTATEHANDLE runtimeRootRule;
    bool everythingIsFine = true;
    everythingIsFine &= SUCCEEDED(grammarToModify->SetGrammarState(SPGS_DISABLED));
    everythingIsFine &= SUCCEEDED(grammarToModify->GetRule(L"rootRule", NULL, SPRAF_TopLevel | SPRAF_Active, TRUE, &runtimeRootRule));
    if(!append)
        everythingIsFine &= SUCCEEDED(grammarToModify->ClearRule(runtimeRootRule));   

    //Build a rule for each vocabulory
    map<string, SPSTATEHANDLE> vocabRules;
    for(map<string, list<string> >::iterator vIt = m_vocabulories.begin(); vIt != m_vocabulories.end(); vIt++)
    {
        //Get the rule name from the key in the dictionary (i.e Agent, Action, etc...)
        std::wstring tmp = s2ws(vIt->first);
        LPCWSTR cwRuleName = tmp.c_str();

        //Get the rule or create it
        everythingIsFine &= SUCCEEDED(grammarToModify->GetRule(cwRuleName, NULL, SPRAF_Dynamic, TRUE, &vocabRules[vIt->first]));
        everythingIsFine &= SUCCEEDED(grammarToModify->ClearRule(vocabRules[vIt->first]));
        for(list<string>::iterator wordIt = vIt->second.begin() ; wordIt != vIt->second.end(); wordIt++)
        {
            std::wstring wordTmp = s2ws(*wordIt);
            LPCWSTR cwWord = wordTmp.c_str();
            everythingIsFine &= SUCCEEDED( grammarToModify->AddWordTransition(vocabRules[vIt->first], NULL, cwWord, NULL, SPWT_LEXICAL, 1, NULL) );
        }
    }

    //Go through the given string and build the according grammar
    //Split the choices
    vector<string> sentences = split(grammar,'|');
    for(vector<string>::iterator it = sentences.begin() ; it != sentences.end() ; it++)
    {
        //Split the words
        vector<string> words = split(*it,' ');            
        SPSTATEHANDLE beforeWordHandle = runtimeRootRule;
        SPSTATEHANDLE afterWordHandle;
        for(vector<string>::iterator itWord = words.begin() ; itWord != words.end() ; itWord++)
        {           
            if((*itWord)=="")
                continue;

            everythingIsFine &= SUCCEEDED(grammarToModify->CreateNewState(beforeWordHandle, &afterWordHandle));

            //Check if the current word is the name of a vocabulory
            if ( (*itWord)[0] == '#' && m_vocabulories.find(*itWord) != m_vocabulories.end())
            {
                everythingIsFine &= SUCCEEDED(grammarToModify->AddRuleTransition(beforeWordHandle, afterWordHandle, vocabRules[*itWord], 1, NULL));
            }
            else
            {            
                std::wstring wordTmp = s2ws(*itWord);
                LPCWSTR cwWord = wordTmp.c_str();
                everythingIsFine &= SUCCEEDED( grammarToModify->AddWordTransition(beforeWordHandle, afterWordHandle, cwWord, NULL, SPWT_LEXICAL, 1, NULL) );
            }
            beforeWordHandle = afterWordHandle;
        }
        everythingIsFine &= SUCCEEDED( grammarToModify->AddWordTransition(beforeWordHandle, NULL, NULL, NULL, SPWT_LEXICAL, 1, NULL) );
    }
    everythingIsFine &= SUCCEEDED(grammarToModify->Commit(NULL));
    everythingIsFine &= SUCCEEDED(grammarToModify->SetGrammarState(SPGS_ENABLED));        
    everythingIsFine &= SUCCEEDED(grammarToModify->SetRuleState(NULL, NULL, SPRS_ACTIVE));
    everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(0));

    return everythingIsFine;
}




/************************************************************************/
bool SpeechRecognizerModule::loadGrammarFromRf(ResourceFinder &RF)
{
    Bottle &bAgent = RF.findGroup("agent");
    Bottle &bAction = RF.findGroup("action");
    Bottle &bObject = RF.findGroup("object");

    Bottle bMessenger, bReply;

    yInfo() << "Agents are: " ;
    for (int iBottle = 1 ; iBottle < bAgent.size() ; iBottle++)
    {
        yInfo() << "\t" << bAgent.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#agent");
        bMessenger.addString(bAgent.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        yInfo() << "\t\t" << bReply.toString() ;
    }

    yInfo() << "\n" << "Actions are: " ;
    for (int iBottle = 1 ; iBottle < bAction.size() ; iBottle++)
    {
        yInfo() << "\t" << bAction.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#action");
        bMessenger.addString(bAction.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        yInfo() << "\t\t" << bReply.toString() ;
    }

    yInfo() << "\n" << "Objects are: " ;
    for (int iBottle = 1 ; iBottle < bObject.size() ; iBottle++)
    {
        yInfo() << "\t" << bObject.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#object");
        bMessenger.addString(bObject.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        yInfo() << "\t\t" << bReply.toString() ;
    }

    Bottle bGrammarMain, bGrammarDef;


    bGrammarMain.addString("addGrammar");
    bGrammarMain.addString("#agent #action #object");

    yInfo() << "\n" << bGrammarMain.toString() ;

//  handleAsyncRecognitionCmd(bGrammarMain, bReply);

    yInfo() << "\n" << bReply.toString() ;

    return true;
}

