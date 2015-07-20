/*
* Copyright(C) 2015 Insitute For Infocomm Research, A*STAR, Singapore
* Authors: Stephane Lallee
* email : stephane.lallee@gmail.com
* Permission is granted to copy, distribute, and / or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* A copy of the license can be found at
* wysiwyd / license / gpl.txt
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General
* Public License for more details
*/

/**
\defgroup windows-tts Windows Speech Synthesizer

\section intro_sec Description
The module based on Microsoft Speech API. 
It is a command line tool to call the Windows speech synthesizer.
Designed to work with iSpeak

\section lib_sec Libraries
Microsoft speech sdk http://msdn.microsoft.com/en-us/library/hh361572(v=office.14).aspx

\section parameters_sec Parameters
 
\section portsa_sec Ports Accessed

\section portsc_sec Interface: Ports Created and data format

\section in_files_sec Input Data Files
None.

\section out_data_sec Output Data Files
None.
 
\section conf_file_sec Configuration Files
None.

\section tested_os_sec Tested OS
Windows 7. 

\author Stephane Lallee 
*/

#include "stdafx.h"

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
    char    *pmbbuf = (char *)malloc(100);
    wcstombs(pmbbuf, s, 100);
    return pmbbuf;
}

////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    //Parse de parameters. Similar way to acapelaCmd
    std::string voice = "iCub_eng";
    if (argc>1)
        voice = argv[1];
    std::cout << "Voice is: " << voice << std::endl;
    std::cout << "TODO : select the right token from this option." << voice << std::endl;

    std::string textInput;
    std::getline(std::cin, textInput);
    std::cout << "Text is: " << textInput << std::endl;

    if (::CoInitializeEx(NULL, COINIT_MULTITHREADED) == S_OK)
    {
        HRESULT hr = S_OK;
        CComPtr<IEnumSpObjectTokens> cpIEnum;
        CComPtr<ISpObjectToken> cpToken;
        CComPtr<ISpVoice> cpVoice;

        // Enumerate voice tokens that speak US English in a female voice.
        hr = SpEnumTokens(SPCAT_VOICES, L"Language=409", L"Gender=Female;", &cpIEnum);

        // Get the best matching token.
        if (SUCCEEDED(hr))
        {
            hr = cpIEnum->Next(1, &cpToken , NULL);
        }

        // Create a voice and set its token to the one we just found.
        if (SUCCEEDED(hr))
        {
            hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
        }

        // Set the voice.
        if (SUCCEEDED(hr))
        {
            hr = cpVoice->SetVoice(cpToken);
        }

        // Set the output to the default audio device.
        if (SUCCEEDED(hr))
        {
            hr = cpVoice->SetOutput(NULL, TRUE);
        }

        // Speak a string directly.
        if (SUCCEEDED(hr))
        {
            hr = cpVoice->Speak(s2ws(textInput).c_str(), NULL, NULL);
        }
    }
    ::CoUninitialize();

    return 0;
}


