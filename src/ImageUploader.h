#pragma once

#include "GPF.h"


		class ImageUploader : public ofThread
		{
		public:
			ImageUploader() {};
			void setup(ofxModule::PostRequestFileSettings s);
			void sendRequest(ofxModule::PostRequestFileSettings s);
			void sendRequest(string filename);
			void threadedFunction();
			ofEvent<ofJson> imageUploadEvent;


		private:
			ofxModule::PostRequestFileSettings uploadSettings;
		};

