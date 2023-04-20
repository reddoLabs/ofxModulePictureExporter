#ifndef _ofxModulePictureExporter
#define _ofxModulePictureExporter

#include "ofMain.h"
#include "ModuleDrawable.h"
#include "ofxTextureRecorder.h"
#include "ImageUploader.h"

#define FFMPEG_FOLDER "ffmpeg/ffmpeg.exe"

namespace ofxModule {

	enum ImageInputType {
		IMAGE,
		TEXTURE,
		FBO,
		MOVIE
	};

	struct ExportSettings {
		string outputType = "jpg";
		int width = 0;
		int height = 0;
		ofScaleMode fit = OF_SCALEMODE_FILL;
		ofImageQualityType quality = OF_IMAGE_QUALITY_BEST;
		ofAlignVert vertAlignment = ofAlignVert::OF_ALIGN_VERT_CENTER;
		ofAlignHorz horzAlignment = ofAlignHorz::OF_ALIGN_HORZ_CENTER;


		// movie Settings
		float framerate = 25;
		ofLoopType loop = OF_LOOP_NONE;
		int nLoops = 1;
	};

	struct ImageExportJob {
		~ImageExportJob() {
			images.clear();
			fbos.clear();
			textures.clear();
			movies.clear();
			exportPaths.clear();
		};
		ImageInputType inputType;
		
		vector<shared_ptr<ofImage>> images;
		vector<shared_ptr<ofFbo>> fbos;
		vector<shared_ptr<ofTexture>> textures;
		
		vector<string> movies;
		int totalFrames;

		vector<string> exportPaths;
		ExportSettings exportSettings;
		string filename;

		ofJson metaData;
	};
	/*
	struct ExportContainer {
		~ExportContainer() {
		};
		ImageExportJob job;
		vector<string> filePaths;
		string path;
	};


	class ImageExportThread : public ofThread {
	public:
		~ImageExportThread() {
			std::unique_lock<std::mutex> lck(mutex);
			containers.clear();
			stopThread();
			condition.notify_all();
		}
		void threadedFunction();
		void addExport(ExportContainer container);

	private:
		std::condition_variable condition;
		deque< ExportContainer> containers;
	};
	*/

// A module for saving and printing images
class ofxModulePictureExporter : public ModuleDrawable {
	
  public:
	ofxModulePictureExporter(string moduleName = "PictureExporter", string settingsPath = "settings.json");
	void draw();
    void update();


	void saveImage(ofImage img, vector <string> path, ofImageQualityType quality);
	
	void copyFiles(ofJson config);
	//void copyFiles(string file, vector<string> dst);

	void exportImage(ModuleEvent& e);
	void exportMovie(ofJson config);

	
	//void printImage(ofImage img, int copies);
	static vector<string> fileCodeToFilename(string code, int nImages);
	static string fileCodeToFilename(string code);

	// image upload
	void setupImageUploader(PostRequestFileSettings settings);
    
protected:
    void proceedModuleEvent(ModuleEvent& e);

	void textureToImage(ImageExportJob& job);
	void fboToImage(ImageExportJob& job);

	void exportJob(ImageExportJob& job);
	void exportImages(ImageExportJob job);
	void exportMp4(ImageExportJob job);
	void readMetaData(ImageExportJob& job, ofJson json);
	static void exportMetaData(ImageExportJob job);
	static void exportMetaData(string path, ofJson json);

	vector<string> createFfmpegCommand(ImageExportJob job);

	static vector<string> matchesInRegex(string _str, string _reg);

	ExportSettings readExportSettings(ofJson json);
	void readPathAndStyleSettings(ImageExportJob& job, ofJson config);

	void updateExportSettings(ExportSettings& settings, ofJson json);


	///// \brief print from windows console command
	//void printConsole(string fileName);

	///// \brief print from IrfanView command line (installed in c:\\irfanview)
	//void printIrfanView(string fileName);

	// video recording
	void startVideoCapture( ofxTextureRecorder::VideoSettings vidSettings,int recorderId = 0);
	void addVideoFrame(ofTexture frame, int recorderId = 0);
	void stopVideoCapture(int recorderId = 0);
	void copyFile(string file, vector<string> dest);
	void copyFile(string file, string dest);

	void onImageUploaded(ofJson& event);
    
private:
	
	vector<string> paths;

	// with texture or fbo
	// convert them to ofPixels in opengl Thread
	// then save them in serperate thread
	shared_ptr<ofTexture> printTexture;
	shared_ptr<ofFbo> printFbo;
	ofPixels printPixels;

	deque< ImageExportJob> jobs;

	string defaultStyle;
	map<string, ExportSettings> styles;

	//mutable std::mutex mutexPaths;
	//std::condition_variable condition;

	map<int,shared_ptr<ofxTextureRecorder>> imageSaver;
	
	std::vector<std::thread> copyThreads;
	long tLastThreadStarted = 0; // timestamp to delete old threads
	int tMaxThreadTime = 30000;

	//ImageExportThread imageExportThread;

	//std::vector<std::thread> imageExportThreads;

	// video record
	map<int,bool> isVideoCapturing;

	ImageUploader imageUploader;
	
};
}
#endif
