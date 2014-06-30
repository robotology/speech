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
    m_timeout = rf.check("timeout",Value(5000)).asInt();
    USE_LEGACY = !rf.check("noLegacy");

    //Deal with speech recognition
    string grammarFile = rf.findFile( rf.check("grammarFile",Value("defaultGrammar.grxml")).asString().c_str() );
    std::wstring tmp = s2ws(grammarFile);
    LPCWSTR cwgrammarfile = tmp.c_str();

    m_useTalkBack = rf.check("talkback");

    //Initialise the speech crap
    bool everythingIsFine = true;
    HRESULT hr;
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine.CoCreateInstance(CLSID_SpInprocRecognizer));
    everythingIsFine &= SUCCEEDED( SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &m_cpAudio));
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine->SetInput(m_cpAudio, TRUE));
    everythingIsFine &= SUCCEEDED( m_cpRecoEngine->CreateRecoContext( &m_cpRecoCtxt ));

    everythingIsFine &=  SUCCEEDED(hr = m_cpRecoCtxt->SetNotifyWin32Event()) ;
    everythingIsFine &= SUCCEEDED(hr = m_cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION))) ;

    //Load grammar from file
    everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 1, &m_cpGrammarFromFile ));
    everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_DISABLED));
    everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));
//  everythingIsFine &= loadGrammarFromRf(rf);

    //Create a runtime grammar
    everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 2, &m_cpGrammarRuntime ));
    everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));

    //Create a dictation grammar
    everythingIsFine &= SUCCEEDED( m_cpRecoCtxt->CreateGrammar( 0, &m_cpGrammarDictation ));
    everythingIsFine &= SUCCEEDED( m_cpGrammarDictation->SetGrammarState(SPGS_DISABLED));
    everythingIsFine &= SUCCEEDED(hr =m_cpGrammarDictation->LoadDictation(NULL, SPLO_STATIC));
    everythingIsFine &= SUCCEEDED(m_cpGrammarDictation->SetDictationState(SPRS_INACTIVE));

    if( everythingIsFine )
    {
        string pName = "/";
        pName += getName();
        pName += "/recog/continuous:o";
        m_portContinuousRecognition.open( pName.c_str() );

        pName = "/";
        pName += getName();
        pName += "/recog/continuousGrammar:o";
        m_portContinuousRecognitionGrammar.open( pName.c_str() );

        //pName = "/";
        //pName += getName();
        //pName += "/iSpeak:o"; 
        //m_port2iSpeak.open( pName.c_str() );
        //if (Network::connect(m_port2iSpeak.getName().c_str(),"/iSpeak"))
        //{
        //  cout<<"Connection to iSpeak succesfull"<<endl;
        //  say("Speech recognizer is running");
        //}
        //else
            cout<<"Unable to connect to iSpeak. Connect manually."<<endl;

        pName = "/";
        pName += getName();
        pName += "/rpc";
        m_portRPC.open( pName.c_str() );
        attach(m_portRPC);

        //Start recognition
        everythingIsFine &= SUCCEEDED(m_cpRecoEngine->SetRecoState(SPRST_ACTIVE_ALWAYS));
        everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION)));
        everythingIsFine &= SUCCEEDED(m_cpGrammarFromFile->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));

        //setRuntimeGrammar_Custom("take the toy|take the banana");
    }

    Bottle bTemp,  bReply;
    bTemp.addString("clear");

//  everythingIsFine &= SUCCEEDED( m_cpGrammarFromFile->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));

    return (everythingIsFine);
}

/************************************************************************/
bool SpeechRecognizerModule::updateModule()
{
    std::cout<<".";
    const float ConfidenceThreshold = 0.3f;
    SPEVENT curEvent;
    ULONG fetched = 0;
    HRESULT hr = S_OK;

    m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);

    while (fetched > 0)
    {
        ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);
        ISpRecoGrammar* resGram = reinterpret_cast<ISpRecoGrammar*>(curEvent.lParam);
        SPPHRASE* pPhrase = NULL;

        //Convert the catched sentence and send it directly through yarp (we loose all the semantic information)
        CSpDynamicString dstrText;
        result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
        string fullSentence = ws2s(dstrText);
        cout<<endl<< fullSentence <<endl;

        int confidence=SP_LOW_CONFIDENCE;

        //Do some smarter stuff with the recognition
        hr = result->GetPhrase(&pPhrase);
        if (SUCCEEDED(hr))
        {
            confidence=pPhrase->Rule.Confidence;

            //--------------------------------------------------- 1. 1st subBottle : raw Sentence -----------------------------------------------//
            int wordCount = pPhrase->Rule.ulCountOfElements;
            string rawPhrase = "";
            for(int i=0; i< wordCount; i++){
                rawPhrase += ws2s(pPhrase->pElements[i].pszDisplayText) + " ";
                std::cout << "word : " <<  ws2s(pPhrase->pElements[i].pszDisplayText) << std::endl;
            }
            std::cout<<"Raw sentence: "<<rawPhrase<<std::endl;

            //--------------------------------------------------- 2. 2nd subottle : Word/Role ---------------------------------------------------//
            Bottle bOutGrammar;
            bOutGrammar.addString(rawPhrase.c_str());
            bOutGrammar.addList()=toBottle(pPhrase,&pPhrase->Rule);
            cout<<"Sending semantic bottle : "<<bOutGrammar.toString()<<endl;
            m_portContinuousRecognitionGrammar.write(bOutGrammar);
            ::CoTaskMemFree(pPhrase);
        }

        Bottle bOut;
        bOut.addString(fullSentence.c_str());
        bOut.addInt(confidence);
        m_portContinuousRecognition.write(bOut);

        if (m_useTalkBack)
            say(fullSentence);

        m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);
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
            for(int i=0; i<siblingRule->ulCountOfElements; i++)
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

    string firstVocab = cmd.get(0).asString().c_str();

    if (firstVocab == "tts")
    {
        string sentence = cmd.get(1).asString().c_str();
        say(sentence);
        reply.addString("ACK");
    }
    else if (firstVocab == "RGM" || firstVocab == "rgm" )
    {    
        string secondVocab = cmd.get(1).asString().c_str();
        if (secondVocab=="vocabulory")
            handleRGMCmd(cmd.tail().tail(), reply);
        reply.addString("ACK");
    }
    else if (firstVocab == "recog")
    {
        handleRecognitionCmd(cmd.tail(), reply);
        reply.addString("ACK");
    }
    else if (firstVocab == "asyncrecog")
    {
        handleAsyncRecognitionCmd(cmd.tail(), reply);
        reply.addString("ACK");
    }
    else
        reply.addString("NACK");

    return true;
}



/************************************************************************/
bool SpeechRecognizerModule::handleRGMCmd(const Bottle& cmd, Bottle& reply)
{
    string firstVocab = cmd.get(0).asString().c_str();
    if (firstVocab == "add")
    {
        string vocabulory = cmd.get(1).asString().c_str();
        if (vocabulory[0] != '#')
        {
            reply.addString("Vocabulories have to start with a #. #Dictation and #WildCard are reserved. Aborting.");
            return true;
        }
        string word = cmd.get(2).asString().c_str();
        m_vocabulories[vocabulory].push_back(word);
        refreshFromVocabulories(m_cpGrammarFromFile);
        reply.addInt(true);
        return true;
    }

    if (firstVocab == "addAuto")
    {
        //todo implement
        refreshFromVocabulories(m_cpGrammarFromFile);
        reply.addInt(true);
        return true;
    }
    reply.addInt(false);
    return false;
}

/************************************************************************/
bool SpeechRecognizerModule::handleAsyncRecognitionCmd(const Bottle& cmd, Bottle& reply)
{
    HRESULT hr;
    string firstVocab = cmd.get(0).asString().c_str();
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
        reply.addInt(everythingIsFine);
        return true;
    }

    if (firstVocab == "addGrammar")
    {    
        string grammar = cmd.get(1).asString().c_str();
        bool everythingIsFine = setGrammarCustom(m_cpGrammarFromFile,grammar,true);
        reply.addInt(everythingIsFine);
        return true;
    }

    if (firstVocab == "loadXML")
    { 
        string xml = cmd.get(1).asString().c_str();
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
        reply.addInt(everythingIsFine);
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
    everythingIsFine &= SUCCEEDED(m_cpRecoCtxt->Resume(0));

    return everythingIsFine;
}

/************************************************************************/
bool SpeechRecognizerModule::handleRecognitionCmd(const Bottle& cmd, Bottle& reply)
{
    string firstVocab = cmd.get(0).asString().c_str();

    if (firstVocab == "timeout")
    {
        m_timeout = cmd.get(1).asInt();
        reply.addInt(true);
        return false;
    }

    else if (firstVocab == "dictation")
    {
        //todo
        cout<<"Dictation is not implemented yet."<<endl;
        return false;
    }    
    else if (firstVocab == "grammarXML")
    {
        string xml = cmd.get(1).asString().c_str();
        ofstream fileTmp("grammarTmp.grxml");
        fileTmp<<xml;
        fileTmp.close();

        std::wstring tmp = s2ws("grammarTmp.grxml");
        LPCWSTR cwgrammarfile = tmp.c_str();

        bool everythingIsFine =true;
        everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));
        everythingIsFine &= SUCCEEDED( m_cpGrammarRuntime->LoadCmdFromFile(cwgrammarfile, SPLO_DYNAMIC));
        everythingIsFine &= SUCCEEDED(m_cpGrammarRuntime->SetRuleState(NULL, NULL, SPRS_ACTIVE));
        
        refreshFromVocabulories(m_cpGrammarRuntime);
        reply.addInt(everythingIsFine);
    }

    else if (firstVocab == "choices")
    {
        string choices ="";
        for (int wI = 1; wI < cmd.size(); wI++)
        {
            choices+=cmd.get(wI).asString().c_str();
            if (wI<cmd.size()-1)
                choices+="|";
        }
        setGrammarCustom(m_cpGrammarRuntime,choices,false);
    }

    else if (firstVocab == "grammarSimple")
    {
        string RADStyle = cmd.get(1).asString().c_str();
        cout<<"Setting runtime grammar to : "<<RADStyle<<endl;
        setGrammarCustom(m_cpGrammarRuntime,RADStyle,false);
    }
    else 
    {
        reply.addString("Wrong syntax");
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
        for(list< pair<string, double> >::iterator it = results.begin(); it != results.end(); it++)
        {
            reply.addString(it->first.c_str());
            reply.addDouble(it->second);
        }
    }
    //Disable the runtime grammar
    SUCCEEDED(m_cpGrammarRuntime->SetGrammarState(SPGS_DISABLED));   
    SUCCEEDED(m_cpGrammarFromFile->SetGrammarState(SPGS_ENABLED));        
    return true;
}
/************************************************************************/
Bottle SpeechRecognizerModule::waitNextRecognition(int timeout)
{
    std::cout<<"Recognition: blocking mode on";
    Bottle bOutGrammar;

    bool gotSomething = false;
    double endTime = Time::now() + timeout/1000.0;
    while(Time::now()<endTime && !gotSomething)
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
            CSpDynamicString dstrText;
            result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
            string fullSentence = ws2s(dstrText);
            cout<<fullSentence<<endl;
            bOutGrammar.addString(fullSentence);

            SPPHRASE* pPhrase = NULL;
            result->GetPhrase(&pPhrase);    
            bOutGrammar.addList() = toBottle(pPhrase,&pPhrase->Rule);
            cout<<"Sending semantic bottle : "<<bOutGrammar.toString()<<endl;
            m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);
        }
    }
    std::cout<<"Recognition: blocking mode off";
    return bOutGrammar;
}

/************************************************************************/
list< pair<string, double> > SpeechRecognizerModule::waitNextRecognitionLEGACY(int timeout)
{
    std::cout<<"Recognition: blocking mode on";
    list< pair<string, double> > recognitionResults;

    bool gotSomething = false;
    double endTime = Time::now() + timeout/1000.0;
    while(Time::now()<endTime && !gotSomething)
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
            cout<<"ON A UN TRUC"<<endl;
            ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);

            //Convert the catched sentence to strings. 
            CSpDynamicString dstrText;
            result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
            string fullSentence = ws2s(dstrText);
            cout<<fullSentence<<endl;
            vector<string> words = split(fullSentence,' ');
            for(int w=0;w<words.size();w++)
            {
                //Todo extract the confidence value somehow...
                recognitionResults.push_back(make_pair(words[w], -1.0));
            }
            m_cpRecoCtxt->GetEvents(1, &curEvent, &fetched);
        }
    }
    std::cout<<"Recognition: blocking mode off";
    return recognitionResults;
}

/************************************************************************/
void SpeechRecognizerModule::say(string s)
{
    cout<<"TTS: "<<s<<endl;
    Bottle b;
    b.addString(s.c_str());
    m_port2iSpeak.write(b);
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
            everythingIsFine &= SUCCEEDED(grammarToModify->CreateNewState(beforeWordHandle, &afterWordHandle));

            //Check if the current word is the name of a vocabulory
            if ( (*itWord)!="" && (*itWord)[0] == '#' && m_vocabulories.find(*itWord) != m_vocabulories.end())
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

    std::cout << "Agents are: " << std::endl;
    for (unsigned int iBottle = 1 ; iBottle < bAgent.size() ; iBottle++)
    {
        std::cout << "\t" << bAgent.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#agent");
        bMessenger.addString(bAgent.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        std::cout << "\t\t" << bReply.toString() << std::endl;
    }

    std::cout << "\n" << "Actions are: " << std::endl;
    for (unsigned int iBottle = 1 ; iBottle < bAction.size() ; iBottle++)
    {
        std::cout << "\t" << bAction.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#action");
        bMessenger.addString(bAction.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        std::cout << "\t\t" << bReply.toString() << std::endl;
    }

    std::cout << "\n" << "Objects are: " << std::endl;
    for (unsigned int iBottle = 1 ; iBottle < bObject.size() ; iBottle++)
    {
        std::cout << "\t" << bObject.get(iBottle).toString();
        bMessenger.clear();
        bMessenger.addString("add");
        bMessenger.addString("#object");
        bMessenger.addString(bObject.get(iBottle).toString());

        handleRGMCmd(bMessenger, bReply);

        std::cout << "\t\t" << bReply.toString() << std::endl;
    }

    Bottle bGrammarMain, bGrammarDef;


    bGrammarMain.addString("addGrammar");
    bGrammarMain.addString("#agent #action #object");

    std::cout << "\n" << bGrammarMain.toString() << std::endl;

//  handleAsyncRecognitionCmd(bGrammarMain, bReply);

    std::cout << "\n" << bReply.toString() << std::endl;

    return true;
}

