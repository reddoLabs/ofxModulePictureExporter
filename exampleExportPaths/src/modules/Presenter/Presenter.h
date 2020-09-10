#ifndef _Presenter
#define _Presenter

#include "ofMain.h"
#include "ModuleDrawable.h"

namespace ofxModule {
	// Basic example of a ModuleDrawable with communication
class Presenter : public ModuleDrawable{
	
  public:
	
	  Presenter(string moduleName = "Presenter");
	
    void update();
	void draw();

	void keyPressed(ofKeyEventArgs & key);
	void keyReleased(ofKeyEventArgs & key) {};
	
protected:
    void proceedModuleEvent(ModuleEvent& e);
    
    private:
		shared_ptr<ofTexture> camImage;
		string text = "1 export default, 2 export square, 3 export small";
};
}

#endif
