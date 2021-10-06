# speechInteraction

This folder includes different modules which use Google Cloud Services.
- **googleSpeechProcess**: it reveals the structure and the syntax of your textual or vocal input by analyzing also the sentiments of your phrases;
- **googleSpeech**: it performs speech-to-text;
- **googleSynthesis**: it performs text-to-speech;
- **googleDialog**: it allows to carry on a conversation about a specific theme, corresponding to your own dialog flow created in your Google project.


In order to use them, the user have to register, create and download a _private key_ from Google Cloud. Below the instructions on how to do:
1. Browse to  [Cloud Console](https://console.cloud.google.com) and create an account. This requires the user to insert a credit card: this process is mandatory in order to run the application, but you will not pay anything
2. Starting from the Dashboard page, under the Project info section, use the button **Go to Project Settings** which will lead you to the Settings
3. Browse to **Service Accounts** using the left navigation bar
4. In the table you will find all the service accounts active for your projects. Use the **Actions** button related to the _dialog flow service_ and select **Create key** choosing the JSON format.

### Some useful information for the user:

In order to inform the user about what is happening in the background while he is interacting with the module, the `/moduleName/state:o` port has been created, which shows different states of the module:

State | Meaning
---- | ----
Listening (_specific for **googleSpeech**_) | Audio is captured by the microphone
Done | The google request has been smoothly performed and the response is not empty
Empty | The google request has been smoothly performed but the response is empty
Failure_\<StatusCode\> |  The google request has not been performed; \<StatusCode\> depends on the type of failure 
Reset (_specific for **googleDialog**_) | The session of the dialogflow has been cleaned
