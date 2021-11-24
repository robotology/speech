

/*
 * Copyright (C) 2018 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff Laura Cavaliere
 * email:  vadim.tikhanoff@iit.it laura.cavaliere@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include <vector>
#include <iostream>
#include <deque>
#include <cstdio>
#include <cmath>

#include <fstream>
#include <iterator>
#include <string>
#include <map>

#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Time.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Semaphore.h>
#include <yarp/sig/SoundFile.h>
#include <yarp/dev/PolyDriver.h>

#include <grpc++/grpc++.h>
#include "google/cloud/language/v1/language_service.grpc.pb.h"

#include "googleSpeechProcess_IDL.h"

using namespace google::cloud::language::v1;
bool is_changed;

static const std::map<grpc::StatusCode, std::string> status_code_to_string {
    {grpc::OK, "ok"},
    {grpc::CANCELLED, "cancelled"},
    {grpc::UNKNOWN, "unknown"},
    {grpc::INVALID_ARGUMENT, "invalid_argument"},
    {grpc::DEADLINE_EXCEEDED, "deadline_exceeded"},
    {grpc::NOT_FOUND, "not_found"},
    {grpc::ALREADY_EXISTS, "already_exists"},
    {grpc::PERMISSION_DENIED, "permission_denied"},
    {grpc::UNAUTHENTICATED, "unauthenticated"},
    {grpc::RESOURCE_EXHAUSTED , "resource_exhausted"},
    {grpc::FAILED_PRECONDITION, "failed_precondition"},
    {grpc::ABORTED, "aborted"},
    {grpc::OUT_OF_RANGE, "out_of_range"},
    {grpc::UNIMPLEMENTED, "unimplemented"},
    {grpc::INTERNAL, "internal"},
    {grpc::UNAVAILABLE, "unavailable"},
    {grpc::DATA_LOSS, "data_loss"},
    {grpc::DO_NOT_USE, "do_not_use"}
};
/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::os::Bottle>
{
    std::string moduleName;
    std::string &state;
    yarp::os::RpcServer handlerPort;
    yarp::os::BufferedPort<yarp::os::Bottle> targetPort;
    
    yarp::os::Bottle wordList;

public:
    /********************************************************/

    Processing( const std::string &moduleName, std::string &state ): state(state)
    {
        this->moduleName = moduleName;
    }

    /********************************************************/
    ~Processing()
    {

    };

    /********************************************************/
    bool open()
    {
        this->useCallback();
        yarp::os::BufferedPort<yarp::os::Bottle >::open( "/" + moduleName + "/text:i" );
        targetPort.open("/"+ moduleName + "/result:o");
        
        //yarp::os::Network::connect("/googleSpeech/result:o", yarp::os::BufferedPort<yarp::os::Bottle >::getName().c_str());

        return true;
    }

    /********************************************************/
    void close()
    {
        yarp::os::BufferedPort<yarp::os::Bottle >::close();
        targetPort.close();
    }

    /********************************************************/
    void onRead( yarp::os::Bottle &bot )
    {
        yarp::os::Bottle &outTargets = targetPort.prepare();

        wordList.clear();
        outTargets.clear();
        outTargets = queryGoogleSyntax(bot);

        targetPort.write();
        yDebug() << "done querying google";
    
    }

    /********************************************************/
    yarp::os::Bottle queryGoogleSyntax(yarp::os::Bottle& text)
    {   
        yarp::os::Bottle b;

        yDebug() << "in queryGoogleSyntax";

        yDebug() << "Phrase is " << text.toString().c_str();
        
        std::string tmp = text.toString();
        
        std::map< std::string, std::string> dictionary;
        dictionary.insert ( std::pair<std::string,std::string>("á","a") );
        dictionary.insert ( std::pair<std::string,std::string>("à","a") );
        dictionary.insert ( std::pair<std::string,std::string>("é","e") );
        dictionary.insert ( std::pair<std::string,std::string>("è","e") );
        dictionary.insert ( std::pair<std::string,std::string>("í","i") );
        dictionary.insert ( std::pair<std::string,std::string>("ó","o") );
        dictionary.insert ( std::pair<std::string,std::string>("ú","u") );
        dictionary.insert ( std::pair<std::string,std::string>("ñ","n") );
        
        std::string tmp2 = tmp;
        std::string strAux;
        for (auto it= dictionary.begin(); it != dictionary.end(); it++)
        {
            tmp2=(it->first);
            std::size_t found=tmp.find_first_of(tmp2);

            while (found!=std::string::npos)
            {
                yError() << "in found" << found;
                strAux=(it->second);
                tmp.erase(found,tmp2.length());
                tmp.insert(found,strAux);
                found=tmp.find_first_of(tmp2,found+1);
            }
        }
        
        yDebug() << "Phrase is now " << tmp.c_str();
        tmp.erase(std::remove(tmp.begin(),tmp.end(),'\"'),tmp.end());
        
        yDebug() << tmp.size();
        yDebug() << std::isalnum(tmp[1]);
        
        if (tmp.size() > 1 && std::isalnum(tmp[0])==0)
            tmp = tmp.substr(1, tmp.size() - 2);
        
        yDebug() << "Phrase is now " << tmp.c_str();

        std::string content = tmp;

        if (content.size()>0){
            AnalyzeSyntaxRequest request;
            AnalyzeSyntaxResponse response;
            grpc::Status status;
            grpc::ClientContext context;

            AnalyzeSentimentRequest sentiment_request;
            AnalyzeSentimentResponse sentiment_response;
            grpc::Status sentiment_status;
            grpc::ClientContext sentiment_context;

            setArguments( request.mutable_document(), content );
            setArguments( sentiment_request.mutable_document(), content );

            // EncodingType //
            request.set_encoding_type( EncodingType::UTF8 );

            b.clear();
            auto creds = grpc::GoogleDefaultCredentials();
            auto channel = grpc::CreateChannel("language.googleapis.com", creds);
            std::unique_ptr< LanguageService::Stub> stub( LanguageService::NewStub( channel ) );

            checkState("Busy");
            yarp::os::Time::delay(0.2);
            status = stub->AnalyzeSyntax( &context, request, &response );
            sentiment_status = stub->AnalyzeSentiment( &sentiment_context, sentiment_request, &sentiment_response );
            std::string status_string = status_code_to_string.at(status.error_code());
            yInfo() << "Status string:" << status_string;
            checkState("Done");

            if ( status.ok() && sentiment_status.ok() ){
                yInfo() << "Status returned OK";
                yInfo() << "\n------Response------\n";

                read_language( &response.language() );
                read_sentences( &response.sentences() );
                read_sentiment( &sentiment_response.sentences() );
                read_tokens( &response.tokens() );
                b = read_analysis( &response.sentences(), &response.tokens(), &sentiment_response.sentences() );

            } 
            else {
                yError() << "Status Returned Cancelled";
                checkState("Failure_" + status_string); 
                yInfo() << status.error_message();
            }
        }  
        else if (content.size()==0) {
            checkState("Empty_input");
        } 
        return b;
    }

    /********************************************************/
    void setArguments(Document* doc, std::string& content)
    {
        doc->set_type( Document_Type::Document_Type_PLAIN_TEXT );
        doc->set_content( content );
    }

    /********************************************************/
    void read_language( const std::string* lang )
    {
        std::cout << "\n----Language----" << std::endl;
        std::cout << "Language: " << *lang << std::endl;
    }

    /********************************************************/
    void read_sentences( const google::protobuf::RepeatedPtrField< Sentence >* sentences ) {

        // SENTENCES //
        yInfo() << "----Sentences----";
        yInfo() << "Sentences Size: " << sentences->size();
        for( int i = 0; i < sentences->size(); i++ ) {
            yInfo() << "Sentence " << i << " has text: " << sentences->Get( i ).has_text();
            if ( sentences->Get( i ).has_text() ) {
                yInfo() << "Sentence text: " << sentences->Get( i ).text().content();
            }
        }
    }

    /********************************************************/
    std::pair<float,float> read_sentiment( const google::protobuf::RepeatedPtrField< Sentence >* sentences ) {
        

        std::pair<float,float> result(9999,9999);
       
        // SENTIMENTS //
        yInfo() << "----Sentiments----";
        yInfo() << "Sentiments Size: " << sentences->size();
        for( int i = 0; i < sentences->size(); i++ ) {
            yInfo() << "Sentiments " << i << " has text: " << sentences->Get( i ).has_text();
            if ( sentences->Get( i ).has_text() ) {
                yInfo() << "Sentiments text: " << sentences->Get( i ).text().content();
            }
            
            yInfo() << "Sentiments " << i << " has sentiment: " << sentences->Get( i ).has_sentiment();
            if ( sentences->Get( i ).has_sentiment() ) {
                yInfo() << "Sentiments " << i << " sentiment: "
                    << "\n\t\tMagnitude: "
                    << sentences->Get( i ).sentiment().magnitude() // float
                    << "\n\t\tScore: "
                    << sentences->Get( i ).sentiment().score() // float
                    << " ";
                    result.first =  sentences->Get( i ).sentiment().magnitude();
                    result.second =  sentences->Get( i ).sentiment().score();
            }
        }
        return result;
    }

    /********************************************************/
    yarp::os::Bottle read_analysis( const google::protobuf::RepeatedPtrField< Sentence >* sentences , const google::protobuf::RepeatedPtrField< Token >* tokens, const google::protobuf::RepeatedPtrField< Sentence >* sentiment_sentences ) {
        

        yarp::os::Bottle &words = wordList.addList();

        for( int i = 0; i < sentences->size(); i++ ) {
            yInfo() << "Sentence " << i << " has text: " << sentences->Get( i ).has_text();
            if ( sentences->Get( i ).has_text() ) {
                yInfo() << "Sentence text: " << sentences->Get( i ).text().content();
                words.addString(sentences->Get( i ).text().content());
            }
        }

        for ( int i = 0; i < tokens->size(); i++ ) {
        
            yarp::os::Bottle &word_analysis = words.addList();
            word_analysis.addString("word");
            word_analysis.addString(tokens->Get( i ).text().content());

            int j = tokens->Get( i ).dependency_edge().head_token_index();
            if ( tokens->Get( j ).text().content() !=  tokens->Get( i ).text().content() ) {
              yarp::os::Bottle &dependency = word_analysis.addList();
              dependency.addString("Dependency");
              dependency.addString(tokens->Get( j ).text().content());
            }

            yarp::os::Bottle &parseLabel = word_analysis.addList();
            parseLabel.addString("Parse_Label");
            parseLabel.addString(DependencyEdge_Label_Name(tokens->Get( i ).dependency_edge().label()));

            yarp::os::Bottle &partOfSpeech = word_analysis.addList();
            partOfSpeech.addString("Part_of_speech");
            partOfSpeech.addString(PartOfSpeech_Tag_Name(tokens->Get( i ).part_of_speech().tag()));

            if ( tokens->Get( i ).lemma() !=  tokens->Get( i ).text().content() ) {
               yarp::os::Bottle &lemma = word_analysis.addList();
               lemma.addString("Lemma");
               lemma.addString(tokens->Get( i ).lemma());
            }

            yarp::os::Bottle &morphology = word_analysis.addList();
            morphology.addString("Morphology");

            yarp::os::Bottle &number = morphology.addList();
            number.addString("number");
            number.addString(PartOfSpeech_Number_Name( tokens->Get( i ).part_of_speech().number() ));

            yarp::os::Bottle &person = morphology.addList();
            person.addString("person");
            person.addString( PartOfSpeech_Person_Name( tokens->Get( i ).part_of_speech().person() ));   

            yarp::os::Bottle &gender = morphology.addList();
            gender.addString("gender");
            gender.addString( PartOfSpeech_Gender_Name( tokens->Get( i ).part_of_speech().gender() ));

            /*yarp::os::Bottle &instance = morphology.addList();
            instance.addString("instance");
            instance.addString(  PartOfSpeech_Case_Name( tokens->Get( i ).part_of_speech().instance() ));*/

            yarp::os::Bottle &tense = morphology.addList();
            tense.addString("tense");
            tense.addString(PartOfSpeech_Tense_Name( tokens->Get( i ).part_of_speech().tense() ));

            yarp::os::Bottle &aspect = morphology.addList();
            aspect.addString("aspect");
            aspect.addString( PartOfSpeech_Aspect_Name( tokens->Get( i ).part_of_speech().aspect() ));

            yarp::os::Bottle &mood = morphology.addList();
            mood.addString("mood");
            mood.addString(PartOfSpeech_Mood_Name( tokens->Get( i ).part_of_speech().mood() ));

            yarp::os::Bottle &voice = morphology.addList();
            voice.addString("voice");
            voice.addString(PartOfSpeech_Voice_Name( tokens->Get( i ).part_of_speech().voice() ));

            yarp::os::Bottle &reciprocity = morphology.addList();
            reciprocity.addString("reciprocity");
            reciprocity.addString(PartOfSpeech_Reciprocity_Name( tokens->Get( i ).part_of_speech().reciprocity() ));

            yarp::os::Bottle &proper = morphology.addList();
            proper.addString("proper");
            proper.addString(PartOfSpeech_Proper_Name( tokens->Get( i ).part_of_speech().proper() ));

            yarp::os::Bottle &form = morphology.addList();
            form.addString("form");
            form.addString( PartOfSpeech_Form_Name( tokens->Get( i ).part_of_speech().form() ));      
        }
        
        
        yarp::os::Bottle &sentiment = words.addList();
        sentiment.addString("Sentiment");

        yarp::os::Bottle &sentiment_score = sentiment.addList();   
        sentiment_score.addString("Score");
        std::pair<float,float> sentiment_result = read_sentiment( sentiment_sentences );
        sentiment_score.addFloat64(sentiment_result.second);

        yarp::os::Bottle &sentiment_magnitude = sentiment.addList();   
        sentiment_magnitude.addString("Magnitude");
        sentiment_magnitude.addFloat64(sentiment_result.first);

        yInfo() << "wordList " << wordList.toString().c_str();
       
        return wordList;
    }

    /********************************************************/
    void read_tokens( const google::protobuf::RepeatedPtrField< Token >* tokens ) {

        for ( int i = 0; i < tokens->size(); i++ ) {
            
            std::cout << "\n-------- " << i << std::endl;
            yInfo() << tokens->Get( i ).text().content();
            yInfo() << "root" << tokens->Get( i ).dependency_edge().head_token_index();
            yInfo() << PartOfSpeech_Tag_Name(tokens->Get( i ).part_of_speech().tag());
            yInfo() << DependencyEdge_Label_Name( tokens->Get( i ).dependency_edge().label() );
            if ( tokens->Get( i ).lemma() ==  tokens->Get( i ).text().content() ) {
              yInfo() << "lemma na";  
            }
            else {
              yInfo() << "lemma" << tokens->Get( i ).lemma();
            }  
        }
        
        for ( int i = 0; i < tokens->size(); i++ ) {

            std::cout
                << "-- Token " << i << " --"
                << std::endl;

            std::cout
                << "Token " << i << " has text: "
                << tokens->Get( i ).has_text()
                << std::endl;

            if ( tokens->Get( i ).has_text() ) {
                std::cout
                    << "\tToken " << i << " text: "
                    << tokens->Get( i ).text().content() // string
                    << "\n\tScore: "
                    << tokens->Get( i ).text().begin_offset() // int32
                    << std::endl;
            }

            std::cout
                << "Token " << i << " has PartOfSpeech: "
                << tokens->Get( i ).has_part_of_speech()
                << std::endl;
            
            if ( tokens->Get( i ).has_part_of_speech() ) {
                std::cout
                << "\n\t\tTag: "
                << PartOfSpeech_Tag_Name( tokens->Get( i ).part_of_speech().tag() )
                    //<< "\n\tToken " << i << " PartOfSpeech: "
                    //<< "\n\t\tAspect: "
                    //<< PartOfSpeech_Aspect_Name( tokens->Get( i ).part_of_speech().aspect() )
                    //<< "\n\t\tCase: "
                    //<< PartOfSpeech_Case_Name( tokens->Get( i ).part_of_speech().instance() )
                    //<< "\n\t\tForm: "
                    //<< PartOfSpeech_Form_Name( tokens->Get( i ).part_of_speech().form() )
                    //<< "\n\t\tGender: "
                    //<< PartOfSpeech_Gender_Name( tokens->Get( i ).part_of_speech().gender() )
                    //<< "\n\t\tMood: "
                    //<< PartOfSpeech_Mood_Name( tokens->Get( i ).part_of_speech().mood() )
                    //<< "\n\t\tNumber: "
                    //<< PartOfSpeech_Number_Name( tokens->Get( i ).part_of_speech().number() )
                    //<< "\n\t\tPerson: "
                    //<< PartOfSpeech_Person_Name( tokens->Get( i ).part_of_speech().person() )
                    //<< "\n\t\tProper: "
                    //<< PartOfSpeech_Proper_Name( tokens->Get( i ).part_of_speech().proper() )
                    //<< "\n\t\tReciprocity: "
                    //<< PartOfSpeech_Reciprocity_Name( tokens->Get( i ).part_of_speech().reciprocity() )
                    //<< "\n\t\tTag: "
                    //<< PartOfSpeech_Tag_Name( tokens->Get( i ).part_of_speech().tag() )
                    //<< "\n\t\tTense: "
                    //<< PartOfSpeech_Tense_Name( tokens->Get( i ).part_of_speech().tense() )
                    //<< "\n\t\tVoice: "
                    //<< PartOfSpeech_Voice_Name( tokens->Get( i ).part_of_speech().voice() )
                
                    << std::endl;
            }

            std::cout
                << "Token " << i << " has DependencyEdge: "
                << tokens->Get( i ).has_dependency_edge()
                << std::endl;

            if ( tokens->Get( i ).has_dependency_edge() ) {
                std::cout
                    << "\tToken " << i << " DependencyEdge: "
                    << "\n\t\tHead Token Index: "
                    << tokens->Get( i ).dependency_edge().head_token_index() // int32
                    << "\n\t\tLabel: "
                    << DependencyEdge_Label_Name( tokens->Get( i ).dependency_edge().label() )
                    << std::endl;
            }
            std::cout
                << "Token " << i << " lemma: "
                << tokens->Get( i ).lemma() // string
                << std::endl
                << std::endl;
        }
    }

    /********************************************************/
    bool start_acquisition()
    {
        return true;
    }

    /********************************************************/
    bool stop_acquisition()
    {
        return true;
    }
     
    /********************************************************/
    yarp::os::Bottle get_dependency()
    {   
        yarp::os::Bottle dependency_result;
        yarp::os::Bottle &dependency_analysis = dependency_result.addList();
        size_t element = wordList.get(0).asList()->size();
        //yInfo() << "element " << element;
        for(size_t i=1; i<element-1; i++){
          yarp::os::Bottle* new_element = wordList.get(0).asList()->get(i).asList(); 
          //printf("dependency check: %s\n", new_element->find("Dependency").asString().c_str());
          yarp::os::Bottle &each_dependency = dependency_analysis.addList();
          each_dependency.addString(new_element->find("word").asString()); //add the word
          each_dependency.addString(new_element->find("Dependency").asString()); //add the corresponding dependency 
        }


        yInfo() << "dependency_result " << dependency_result.toString().c_str();// ((This is) (is "n/a") (a test) (test is)) 

        return dependency_result;
    }

    /********************************************************/
    yarp::os::Bottle get_parseLabel()
    {   
        yarp::os::Bottle parseLabel_result;
        yarp::os::Bottle &parseLabel_analysis = parseLabel_result.addList();
        size_t element = wordList.get(0).asList()->size();
        for(size_t i=1; i<element-1; i++){
          yarp::os::Bottle* new_element = wordList.get(0).asList()->get(i).asList(); 
          yarp::os::Bottle &each_parseLabel = parseLabel_analysis.addList();
          each_parseLabel.addString(new_element->find("word").asString()); //add the word
          each_parseLabel.addString(new_element->find("Parse_Label").asString()); //add the corresponding parseLabel
        }


        yInfo() << "parseLabel_result " << parseLabel_result.toString().c_str();

        return parseLabel_result;
    }

   /********************************************************/
    yarp::os::Bottle get_partOfSpeech()
    {   
        yarp::os::Bottle partOfSpeech_result;
        yarp::os::Bottle &partOfSpeech_analysis = partOfSpeech_result.addList();
        size_t element = wordList.get(0).asList()->size();
        for(size_t i=1; i<element-1; i++){
          yarp::os::Bottle* new_element = wordList.get(0).asList()->get(i).asList(); 
          yarp::os::Bottle &each_partOfSpeech = partOfSpeech_analysis.addList();
          each_partOfSpeech.addString(new_element->find("word").asString()); //add the word
          each_partOfSpeech.addString(new_element->find("Part_of_speech").asString()); //add the corresponding partOfSpeech
        }


        yInfo() << "partOfSpeech_result " << partOfSpeech_result.toString().c_str();

        return partOfSpeech_result;
    }

   /********************************************************/
    yarp::os::Bottle get_lemma()
    {   
        yarp::os::Bottle lemma_result;
        yarp::os::Bottle &lemma_analysis = lemma_result.addList();
        size_t element = wordList.get(0).asList()->size();
        for(size_t i=1; i<element-1; i++){
          yarp::os::Bottle* new_element = wordList.get(0).asList()->get(i).asList(); 
          yarp::os::Bottle &each_lemma = lemma_analysis.addList();
          each_lemma.addString(new_element->find("word").asString()); //add the word
          each_lemma.addString(new_element->find("Lemma").asString()); //add the corresponding Lemma
        }


        yInfo() << "lemma_result " << lemma_result.toString().c_str();

        return lemma_result;
    }

   /********************************************************/
    yarp::os::Bottle get_morphology()
    {   
        yarp::os::Bottle morphology_result;
        yarp::os::Bottle &morphology_analysis = morphology_result.addList();
        size_t element = wordList.get(0).asList()->size();
        for(size_t i=1; i<element-1; i++){
          yarp::os::Bottle* new_element = wordList.get(0).asList()->get(i).asList();//contains the word with all the speech-features
          yarp::os::Bottle* morphology = new_element->get(new_element->size()-1).asList();  
          //yInfo() << "morphology " << morphology->toString();// Morphology (number NUMBER_UNKNOWN) (person PERSON_UNKNOWN) (gender GENDER_UNKNOWN) etc
          yarp::os::Bottle &each_morphology = morphology_analysis.addList();
          each_morphology.addString(new_element->find("word").asString()); //add the word
	  for( size_t i=1; i<morphology->size()-1; i++) {
  		yarp::os::Bottle &each_value = each_morphology.addList();
  		each_value.addString(morphology->get(i).asList()->get(0).toString());//add (number NUMBER_UNKNOWN) (person PERSON_UNKNOWN) etc
                each_value.addString(morphology->get(i).asList()->get(1).toString());
	  }
        }


        yInfo() << "morphology_result " << morphology_result.toString().c_str();

        return morphology_result;
    }

    /********************************************************/
    yarp::os::Bottle get_sentiment()
    {   
        yarp::os::Bottle sentiment_result;
        yarp::os::Bottle &sentiment_analysis = sentiment_result.addList();
        size_t element = wordList.get(0).asList()->size();
        yarp::os::Bottle* first_element = wordList.get(0).asList();
        std::string sentence = first_element->get(0).asString(); 
        //yInfo() << "sentence" << sentence;
        sentiment_analysis.addString(sentence); //add the sentence
        yarp::os::Bottle* sentiment = wordList.get(0).asList()->get(element-1).asList(); 
        //yInfo() << "sentiment " << sentiment->toString();//Sentiment (Score 0.100000001490116119385) (Magnitude 0.100000001490116119385) 
        double score = sentiment->get(1).asList()->get(1).asFloat64();
        double magnitude = sentiment->get(2).asList()->get(1).asFloat64();
        //printf("Values: %f %f\n", score, magnitude);
        yarp::os::Bottle &each_score = sentiment_analysis.addList();
        each_score.addString("Score"); //add the corresponding score
        each_score.addFloat64(score);
        yarp::os::Bottle &each_magnitude = sentiment_analysis.addList();
        each_magnitude.addString("Magnitude"); //add the corresponding magnitude
        each_magnitude.addFloat64(magnitude);
        


        yInfo() << "sentiment_result " << sentiment_result.toString().c_str();// ("You make me very angry" (Score -0.899999976158142089844) (Magnitude 0.899999976158142089844)) 


        return sentiment_result;
    }

    /********************************************************/
    bool checkState(std::string new_state)
    {
        if(new_state!=state){
            is_changed=true;
            state=new_state;
        }
        else{
            is_changed=false;
        }
        return is_changed;
    }
};

/********************************************************/
class Module : public yarp::os::RFModule, public googleSpeechProcess_IDL
{
    yarp::os::ResourceFinder *rf;
    yarp::os::RpcServer rpcPort;
    std::string state;
    yarp::os::BufferedPort<yarp::os::Bottle> statePort;


    Processing                  *processing;
    friend class                processing;

    bool                        closing;

    /********************************************************/
    bool attach(yarp::os::RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

public:

    /********************************************************/
    bool configure(yarp::os::ResourceFinder &rf)
    {
        this->rf=&rf;
        this->state="Ready";
        std::string moduleName = rf.check("name", yarp::os::Value("yarp-google-speech-process"), "module name (string)").asString();

        setName(moduleName.c_str());

        rpcPort.open(("/"+getName("/rpc")).c_str());
        statePort.open("/"+ moduleName + "/state:o");

        closing = false;

        processing = new Processing( moduleName, state );

        /* now start the thread to do the work */
        processing->open();

        attach(rpcPort);

        return true;
    }

    /**********************************************************/
    bool close()
    {   
        statePort.close();
        processing->close();
        delete processing;
        return true;
    }

    /**********************************************************/
    bool start()
    {
        processing->start_acquisition();
        return true;
    }

    /**********************************************************/
    bool stop()
    {
        processing->stop_acquisition();
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /********************************************************/
    bool quit()
    {
        closing=true;
        return true;
    }

    /********************************************************/
    bool updateModule()
    { 
        if(is_changed){
            is_changed=false;
            yarp::os::Bottle &outTargets = statePort.prepare();
            outTargets.clear();
            outTargets.addString(state);
            yDebug() << "outTarget:" << outTargets.toString().c_str();
            statePort.write();
        }
        return !closing;
    }

    /********************************************************/
    yarp::os::Bottle get_dependency()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_dependency();

	return answer;
    }

    /********************************************************/
    yarp::os::Bottle get_parseLabel()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_parseLabel();

	return answer;
    }

    /********************************************************/
    yarp::os::Bottle get_partOfSpeech()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_partOfSpeech();

	return answer;
    }
    
    /********************************************************/
    yarp::os::Bottle get_lemma()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_lemma();

	return answer;
    }

    /********************************************************/
    yarp::os::Bottle get_morphology()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_morphology();

	return answer;
    }

    /********************************************************/
    yarp::os::Bottle get_sentiment()
    {   
        yarp::os::Bottle answer;
        answer = processing->get_sentiment();

	return answer;
    }

};

/********************************************************/
int main(int argc, char *argv[])
{
    yarp::os::Network::init();

    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("YARP server not available!");
        return 1;
    }

    Module module;
    yarp::os::ResourceFinder rf;

    rf.setVerbose( true );
    rf.setDefaultContext( "googleSpeechProcess" );
    rf.setDefaultConfigFile( "config.ini" );
    rf.setDefault("name","googleSpeechProcess");
    rf.configure(argc,argv);

    return module.runModule(rf);
}
