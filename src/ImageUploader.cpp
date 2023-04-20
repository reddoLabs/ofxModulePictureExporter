#include "ImageUploader.h"


void ImageUploader::setup(ofxModule::PostRequestFileSettings s)
{
	uploadSettings = s;
}

void ImageUploader::sendRequest(ofxModule::PostRequestFileSettings s)
{
	uploadSettings = s;
	startThread();
}

void ImageUploader::sendRequest(string filename)
{
	uploadSettings.filename = filename;
	startThread();
}

void ImageUploader::threadedFunction()
{
	auto response = ofxModule::GPF::sendPostRequestFile(uploadSettings);
	ofJson j;
	
	// try again once if upload does not work
	if (response.status != 200) {
		ofSleepMillis(1000);
		response = ofxModule::GPF::sendPostRequestFile(uploadSettings);
	}
	j["status"] = response.status;
	ofStringReplace(uploadSettings.filename, "\\", "/");
	j["file"] = ofSplitString(uploadSettings.filename, "/").back();

	ofNotifyEvent(imageUploadEvent, j);
	ofSleepMillis(1000);
}

