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
		ofSetColor(255);
		if (camImage != nullptr) camImage->draw(0, 0, 640, 480);

		ofDrawBitmapStringHighlight(text, 20, 20);


		// capture current image
		if (isCapturing) {
			ofSetColor(255, 0, 0);
			ofDrawCircle(30, 50, 10);

			ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","frame"} }));
			e.destClass = "PictureExporter";
			e.texture = camImage;
			notifyEvent(e);
		}
		
	}

	void Presenter::keyPressed(ofKeyEventArgs & key)
	{
		switch (key.key)
		{
		case ' ': {
			if (!isCapturing) {
				//start capture
				ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","start"} }));
				e.texture = camImage;
				e.destClass = "PictureExporter";
				notifyEvent(e);
			}
			else {
				// stop capture
				ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","stop"} }));
				e.destClass = "PictureExporter";
				notifyEvent(e);
			}

			isCapturing = !isCapturing;
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
