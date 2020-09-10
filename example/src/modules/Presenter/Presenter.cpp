#include "Presenter.h"
#include "GPF.h"

namespace ofxModule {
    
	Presenter::Presenter(string moduleName):ModuleDrawable("Presenter",moduleName){
		ofRegisterKeyEvents(this);
    }
  
    
    
    //------------------------------------------------------------------
    void Presenter::update() {

    }

	void Presenter::draw()
	{
		// draw the camera image
		if (camImage != nullptr) camImage->draw(0, 0, 640, 480);

		ofDrawBitmapStringHighlight(text, 20, 20);
	}

	void Presenter::keyPressed(ofKeyEventArgs & key)
	{
		switch (key.key)
		{
		case '1': {
			//save an ofImage
			ofPixels p;
			ofFbo fbo;
			fbo.allocate(camImage->getWidth(), camImage->getHeight());
			fbo.begin();
			camImage->draw(0, 0);
			fbo.end();
			fbo.readToPixels(p);
			shared_ptr<ofImage> img = shared_ptr<ofImage>(new ofImage());
			img->setFromPixels(p);

			ofJson msg;
			msg["filename"] = "export_ofImage.png";
			notifyEvent(img,"exportImage", msg);
			break;
		}
		case '2': {
			//save ofTexture
			ofJson msg;
			msg["filename"] = "export_ofTexture.png";
			notifyEvent(camImage,"exportImage", msg);
			break;
		}
		case '3': {
			//save ofFbo
			shared_ptr<ofFbo> fbo = shared_ptr<ofFbo>(new ofFbo());
			fbo->allocate(camImage->getWidth(), camImage->getHeight());
			fbo->begin();
			camImage->draw(0, 0);
			fbo->end();

			ofJson msg;
			msg["path"] = "export_ofFbo.png";
			notifyEvent(fbo,"exportImage", msg);
			break;
		}
		default:
			break;
		}
	}
    
    //------------------------------------------------------------------
    void Presenter::proceedModuleEvent(ModuleEvent& e) {
		
		// update the cam image 
		if (e.type == ModuleEvent::MODULE_EVENT_TEXTURE) {
			camImage = e.texture;
		}
    }
    
}
