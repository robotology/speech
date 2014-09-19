/*
 * AcaTTS Sample
 *
 * Copyright (c) 2010 Acapela Group
 *
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
*/

#include <string>
#include <iostream>
#include <iomanip>
#include <windows.h>

#include "ioBabTts.h"

#define _BABTTSDYN_IMPL_
#include "ifBabTtsDyn.h"
#undef _BABTTSDYN_IMPL_


int main(int argc, char *argv[])
{
    std::string voice="iCub_eng";
    if (argc>1)
        voice=argv[1];

    std::cout<<"Voice is: "<<voice<<std::endl;

    char textInput[256];
    std::cin.getline(textInput,sizeof(textInput));
    std::cout<<"Text is: "<<textInput<<std::endl;

    LPBABTTS lpEngine;    
    if (BabTtsInitDllEx(_T("."))==NULL)
    {
        std::cerr<<"Error: Could not load AcaTTS"<<std::endl;
        exit(-1);
    }
    
    if (!BabTTS_Init())
    {
        std::cerr<<"Error: Could not initialize AcaTTS"<<std::endl;
        BabTtsUninitDll();
        exit(-1);
    }
    
    lpEngine=BabTTS_Create(); 
    if (lpEngine==NULL)
    {
        std::cerr<<"Error: Could not create AcaTTS context"<<std::endl;
        BabTTS_Uninit();
        BabTtsUninitDll();
        exit(-1);
    }
    
    if (BabTTS_Open(lpEngine,voice.c_str(),0)!=E_BABTTS_NOERROR)
    {
        std::cerr<<"Error: Could not open "<<voice<<std::endl;
        BabTTS_Close(lpEngine);
        BabTTS_Uninit();
        BabTtsUninitDll();
        exit(-1);
    }

    if (argc>2)
    {
        int speed=atoi(argv[2]);
        BabTTS_SetSettings(lpEngine,BABTTS_PARAM_SPEED,speed);
    }

    if (argc>3)
    {
        int pitchMultiplier=atoi(argv[3]);
        BabTTS_SetSettings(lpEngine,BABTTS_PARAM_PITCH,pitchMultiplier);
    }    

    BabTTS_Speak(lpEngine,textInput,BABTTS_SYNC|BABTTS_TAG_SAPI);

    BabTTS_Close(lpEngine);
    BabTTS_Uninit();
    BabTtsUninitDll();
    
    return 0;
}
