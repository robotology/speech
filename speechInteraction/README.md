# speechInteraction

This folder includes different modules which use google cloud services.
- **googleSpeechProcess**: it reveal the structure and syntax of your textual or vocal input by analyzing also the sentiments of your phrases;
- **googleSpeech**:  it performs speech-to-text;
- **googleSynthesis**: it performs text-to-speech;
- **googleDialog**: it allows to carry on a conversation about a specific world, corresponding to your own dialog flow created in your Google project.


In order to use them, the user have to register and download the private key from Google Cloud. Below the instructions on how to do:
1. Browse to  [Cloud Console](https://console.cloud.google.com) and create an account. This requires the user to insert a credit card: this process is mandatory in order to run the application, but you will not pay anything
2. Starting from the Dashboard page, under the Project info section, use the button **Go to Project Settings** which will lead you to the Settings
3. Browse to **Service Accounts** using the left navigation bar
4. In the table you will find all the service accounts active for your projects. Use the **Actions** button related to the _dialog flow service_ and select **Create key** choosing the JSON format.
