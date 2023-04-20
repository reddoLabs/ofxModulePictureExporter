#include "ofxModulePictureExporter.h"
#include <regex>

namespace ofxModule {
    //------------------------------------------------------------------
	ofxModulePictureExporter::ofxModulePictureExporter(string moduleName, string settingsPath):ModuleDrawable("PictureExporter", moduleName,settingsPath) {
		defaultStyle = settings["defaultStyle"].get<string>();
		for (auto& style:settings["styles"]) {
			styles.insert(pair<string, ExportSettings>(style["id"], readExportSettings(style)));
		}

		// init image uploader
		if (settings["upload"].is_object()) {
			PostRequestFileSettings imageUploadSettings;
			imageUploadSettings.timeout = settings["upload"]["timeout"].is_null() ? 3 : settings["upload"]["timeout"].get<int>();
			imageUploadSettings.token = settings["upload"]["token"].is_null() ? "" : settings["upload"]["token"].get<string>();
			imageUploadSettings.url = settings["upload"]["url"].is_null() ? "" : settings["upload"]["url"].get<string>();
			imageUploadSettings.user = settings["upload"]["user"].is_null() ? "" : settings["upload"]["user"].get<string>();

			if (settings["upload"]["header"].is_array()) {
				for (auto header : settings["upload"]["header"]) {
					imageUploadSettings.header.insert(make_pair(header[0].get<string>(), header[1].get<string>()));
				}
			}
			imageUploader.setup(imageUploadSettings);
			ofAddListener(imageUploader.imageUploadEvent, this, &ofxModulePictureExporter::onImageUploaded);
		}
		



		//imageExportThread.startThread();
    }

	void ofxModulePictureExporter::draw()
	{
	}
    
    //------------------------------------------------------------------
    void ofxModulePictureExporter::update() {
		if (jobs.size() > 0) {
			exportJob(jobs.front());
			jobs.pop_front();
		}
    }

	void ofxModulePictureExporter::saveImage(ofImage img, vector<string> path, ofImageQualityType quality)
	{
		for (auto& p : path) {
			ofSaveImage(img, p,quality);
		}
		
	}

	void ofxModulePictureExporter::copyFiles( ofJson config) {
		// file to copy
		string baseFile = config["src"].get<string>();

		// new file name
		string filename;
		if (config["filename"].is_null()) {
			filename = "[yy]-[mm]-[dd]-[s]";
		} else {
			filename = config["filename"].get<string>();
		}
		// ending
		filename += "." + ofSplitString(baseFile, ".").back();
		filename = fileCodeToFilename(filename);

		// get export paths
		vector<string> exportPaths;
		if (!config["paths"].is_null()) {
			if (config["paths"].is_array()) {
				for (auto& p : config["paths"]) {
					exportPaths.push_back(p);
				}
			} else {
				exportPaths.push_back(config["paths"].get<string>());
			}
		}

		baseFile = ofFilePath::getAbsolutePath(baseFile);

		// proceed copying
		for (auto& thread : copyThreads) {
			thread.join();
		}
		copyThreads.clear();

		for (auto& path : exportPaths) {
			exportMetaData(path, config);
			path += "/" + filename;
			string fTemp = ofFilePath::getAbsolutePath(path);
			ofStringReplace(fTemp, "/", "\\");

			copyThreads.push_back(std::thread([baseFile, fTemp] {
				string cmd = "copy " + baseFile + " " + fTemp;
				ofSystem(cmd);
			}));
		}
	}


	void ofxModulePictureExporter::exportImage(ModuleEvent & e) {
		ImageExportJob s;
		readMetaData(s, e.message);

		// get input
		if (e.images.size() > 0 || e.image != nullptr) {
			s.inputType = IMAGE;
			if (e.image != nullptr) s.images.push_back(e.image);
			for (auto& i : e.images) {
				s.images.push_back(i);
			}
		} else if (e.textures.size() > 0 || e.texture != nullptr) {
			s.inputType = TEXTURE;
			if (e.texture != nullptr) s.textures.push_back(e.texture);
			for (auto& i : e.textures) {
				s.textures.push_back(i);
			}
		} else if (e.fbos.size() > 0 || e.fbo != nullptr) {
			s.inputType = FBO;
			if (e.fbo != nullptr) s.fbos.push_back(e.fbo);
			for (auto& i : e.fbos) {
				s.fbos.push_back(i);
			}
		}

		readPathAndStyleSettings(s, e.message);
		exportImages(s);
	}

	void ofxModulePictureExporter::exportMovie(ofJson config) {
		// read input files
		ImageExportJob s;
		readMetaData(s, config);

		if (config["src"].is_array()) {
			std::vector<std::string> sTemp = config["src"];
			s.movies = sTemp;
		} else {
			s.movies.push_back(config["src"].get<string>());
		}
		s.totalFrames = config["nFrames"];

		// get export paths and styles
		readPathAndStyleSettings(s, config);

	

		// proceed copying
		
		if (ofGetElapsedTimeMillis() - tLastThreadStarted > tMaxThreadTime) {
			for (auto& thread : copyThreads) {
				thread.join();
			}
			copyThreads.clear();
		}

		//createFfmpegCommand(s);
		exportMetaData(s);
		for (auto cmd : createFfmpegCommand(s)) {
			tLastThreadStarted = ofGetElapsedTimeMillis();
			copyThreads.push_back(std::thread([cmd] {
				ofSystem(cmd);
			}));
		}
	}

	///\brief replace all codes with values
	vector<string> ofxModulePictureExporter::fileCodeToFilename(string code, int nImages) {
		vector < string> ret;
		ofStringReplace(code, "[y]", ofToString(ofGetYear()).substr(2));
		ofStringReplace(code, "[yy]", ofToString(ofGetYear()));
		ofStringReplace(code, "[m]", ofToString(ofGetMonth()));
		ofStringReplace(code, "[mm]", ofGetMonth() < 10 ? "0" + ofToString(ofGetMonth()) : ofToString(ofGetMonth()));
		ofStringReplace(code, "[d]", ofToString(ofGetDay()));
		ofStringReplace(code, "[dd]", ofGetDay() < 10 ? "0" + ofToString(ofGetDay()) : ofToString(ofGetDay()));
		ofStringReplace(code, "[h]", ofToString(ofGetHours()));
		ofStringReplace(code, "[hh]", ofGetHours() < 10 ? "0" + ofToString(ofGetHours()) : ofToString(ofGetHours()));
		ofStringReplace(code, "[M]", ofToString(ofGetMinutes()));
		ofStringReplace(code, "[MM]", ofGetMinutes() < 10 ? "0" + ofToString(ofGetMinutes()) : ofToString(ofGetMinutes()));
		ofStringReplace(code, "[S]", ofToString(ofGetSeconds()));
		ofStringReplace(code, "[SS]", ofGetMinutes() < 10 ? "0" + ofToString(ofGetSeconds()) : ofToString(ofGetSeconds()));
		ofStringReplace(code, "[i]", ofGetTimestampString("%i"));
		ofStringReplace(code, "[s]", ofToString(ofGetHours()*3600 + ofGetMinutes()*60 + ofGetSeconds()));


		auto numberPlaceholders = matchesInRegex(code, "\\[(n*)\\]");
		for (size_t i = 0; i < nImages; i++) {
			string retTemp = code;
			for (auto placeholder : numberPlaceholders) {
				string number = ofToString(i);
				if (placeholder.length() - number.length() > 0) {
					for (size_t i = 0; i < placeholder.length() - number.length(); i++) {
						number = "0" + number;
					}
				}
				ofStringReplace(retTemp, placeholder, number);
			}
			ret.push_back(retTemp);
		}
		
		return ret;
	}

	string ofxModulePictureExporter::fileCodeToFilename(string code) {
		return fileCodeToFilename(code,1)[0];
	}

	void ofxModulePictureExporter::proceedModuleEvent(ModuleEvent & e)
	{
		if (e.address == "exportImage") {
			exportImage(e);
		}
		else if (e.address == "exportMovie") {
			exportMovie(e.message);
		}
		else if (e.address == "capture") {
			int recorderId = 0;
			if (e.message["recorderId"] != nullptr) recorderId = e.message["recorderId"];
			int fps = e.message["framerate"] != nullptr ? e.message["fps"] : settings["capture"]["framerate"];

			ofTexture tSample;

			if (e.message["option"].get<string>() == "start") {

				if (e.fbos.size() > 0 || e.fbo != nullptr) {
					if (e.fbo != nullptr) {
						tSample = e.fbo->getTexture();
					}
					else {
						tSample = e.fbos.front()->getTexture();
					}
				}
				else if (e.textures.size() > 0 || e.texture != nullptr) {
					if (e.texture != nullptr) {
						tSample = *e.texture;
					}
					else {
						tSample = e.fbos.front()->getTexture();
					}
				}
				else if (e.images.size() > 0 || e.image != nullptr) {
					if (e.image != nullptr) {
						tSample = e.image->getTexture();
					}
					else {
						tSample = e.images.front()->getTexture();
					}
				}
				else {
					ofFbo fbo;
					fbo.allocate(e.message["width"], e.message["height"]);
					tSample = fbo.getTexture();
				}

				ofxTextureRecorder::VideoSettings s(tSample,fps);
				s.bitrate = e.message["bitrate"] != nullptr ? e.message["bitrate"].get<string>() : settings["capture"]["bitrate"].get<string>();
				s.videoPath = e.message["file"] != nullptr ? e.message["file"].get<string>() : settings["capture"]["file"].get<string>();
				if (recorderId != 0) {
					auto pTemp = ofSplitString(s.videoPath, ".");
					s.videoPath = pTemp.front() + ofToString(recorderId) + "." + pTemp.back();
				}
				s.videoCodec = e.message["bitrate"] != nullptr ? e.message["codec"].get<string>() : settings["capture"]["codec"].get<string>();
				s.extrasettings = e.message["extraSettings"] != nullptr ? e.message["extraSettings"].get<string>() : (settings["capture"]["extraSettings"] != nullptr ? settings["capture"]["extraSettings"].get<string>() : "");

				s.numThreads = 4;
				s.maxMemoryUsage = 9000000000;

				startVideoCapture(s,recorderId);

			} else if (e.message["option"].get<string>() == "frame") {
				if (e.fbos.size() > 0 || e.fbo != nullptr) {
					if (e.fbo != nullptr) {
						addVideoFrame(e.fbo->getTexture(), recorderId);
					}
					else {
						addVideoFrame(e.fbos.front()->getTexture(), recorderId);
					}
				}else if (e.textures.size() > 0 || e.texture != nullptr) {
					if (e.texture != nullptr) {
						addVideoFrame(*e.texture, recorderId);
					}
					else {
						addVideoFrame(*e.textures.front(), recorderId);
					}
				}
				else if (e.images.size() > 0 || e.image != nullptr) {
					if (e.image != nullptr) {
						addVideoFrame(e.image->getTexture(), recorderId);
					}
					else {
						addVideoFrame(e.images.front()->getTexture(), recorderId);
					}
				}
					
			} else if (e.message["option"].get<string>() == "stop") {
				stopVideoCapture(recorderId);
			}
		} else if (e.address == "copy") {
			copyFiles(e.message);
		}
	}
	void ofxModulePictureExporter::textureToImage(ImageExportJob & job) {
		for (auto& tex : job.textures) {
			job.images.push_back(make_shared<ofImage>(ofImage()));
			ofPixels p;
			tex->readToPixels(p);
			job.images.back()->setFromPixels(p);
		}
		job.textures.clear();
		job.inputType = IMAGE;
	}

	void ofxModulePictureExporter::fboToImage(ImageExportJob & job) {
		for (auto& fbo : job.fbos) {
			job.images.push_back(make_shared<ofImage>(ofImage()));
			ofPixels p;
			fbo->readToPixels(p);
			job.images.back()->setFromPixels(p);
		}
		job.fbos.clear();
		job.inputType = IMAGE;
	}

	void ofxModulePictureExporter::exportJob(ImageExportJob & job) {
		// export each path as a single job
		ImageExportJob tJob = job;
		if (job.exportSettings.outputType == "png" || job.exportSettings.outputType == "jpg") {
			exportImages(tJob);
		}
		else {
			exportMp4(tJob);
		}
		
	}

	void ofxModulePictureExporter::exportImages(ImageExportJob job) {
		vector<string> filePaths;
		
		exportMetaData(job);

		// clear image export threads
		/*for (auto& thread : imageExportThreads) {
			thread.join();
		}
		imageExportThreads.clear();*/

		// get filepaths
		int nImages;
		switch (job.inputType) {
			case IMAGE:
				nImages = job.images.size(); 
				filePaths = fileCodeToFilename(job.filename, job.images.size());
				break;
			case TEXTURE: 
				nImages = job.textures.size(); 
				filePaths = fileCodeToFilename(job.filename, job.textures.size());
				break;
			case FBO: 
				nImages = job.fbos.size(); 
				filePaths = fileCodeToFilename(job.filename, job.fbos.size());
				break;
		}
		
		// proceed export jobs	
		if (job.exportPaths.size() == 0) {
			job.exportPaths.push_back("");
		}
		for (auto& path : job.exportPaths) {

			int width;
			int height;
			switch (job.inputType) {
			case IMAGE:
				width = job.images.front()->getWidth();
				height = job.images.front()->getHeight();
				break;
			case TEXTURE:
				width = job.textures.front()->getWidth();
				height = job.textures.front()->getHeight();
				break;
			case FBO:
				width = job.fbos.front()->getWidth();
				height = job.fbos.front()->getHeight();
				break;
			}

			if (job.exportSettings.width != 0) width = job.exportSettings.width;
			if (job.exportSettings.height != 0) height = job.exportSettings.height;

			switch (job.inputType) {
			case IMAGE:
				for (int i = 0; i < nImages; ++i) {
					ofFbo fbo;
					fbo.allocate(job.images[i]->getWidth(), job.images[i]->getHeight());
					fbo.begin();
					ofClear(0, 0);
					ofSetColor(255);
					job.images[i]->draw(0, 0);
					fbo.end();
					job.fbos.push_back(shared_ptr<ofFbo>(new ofFbo(fbo)));

				}
				break;
			case TEXTURE:
			{
				for (int i = 0; i < nImages; ++i) {
					ofFbo fbo;
					fbo.allocate(job.textures[i]->getWidth(), job.textures[i]->getHeight());
					fbo.begin();
					ofClear(0, 0);
					ofSetColor(255);
					job.textures[i]->draw(0, 0);
					fbo.end();
					job.fbos.push_back(shared_ptr<ofFbo>(new ofFbo(fbo)));
				}
				break;
			}
			case FBO:
				break;
			}

				for (int i = 0; i < nImages; ++i) {
					string filename = filePaths[i] + ".";
					filename += job.exportSettings.outputType;

					string exportPath;
					exportPath = path +"/" + filename;

					// resize and crop
					if (job.fbos[i]->getWidth() != width || job.fbos[i]->getHeight() != height) {

						ofRectangle in{ 0,0,job.fbos[i]->getWidth(),job.fbos[i]->getHeight() };
						ofRectangle out{ 0,0,float(width),float(height) };
						ofFbo outFbo;
						outFbo.allocate(out.width, out.height);
						outFbo.begin();
						ofClear(0, 0);
						switch (job.exportSettings.fit)
						{
						case OF_SCALEMODE_FIT:
							in.scaleTo(out, OF_ASPECT_RATIO_KEEP, job.exportSettings.horzAlignment, job.exportSettings.vertAlignment);
							job.fbos[i]->draw(in);
							break;
						case OF_SCALEMODE_FILL:
							in.scaleTo(out, OF_SCALEMODE_FILL);
							if (job.exportSettings.horzAlignment == OF_ALIGN_HORZ_LEFT) {
								in.x = 0;
							}
							else if (job.exportSettings.horzAlignment == OF_ALIGN_HORZ_RIGHT) {
								in.x *= 2;
							}
							if (job.exportSettings.vertAlignment == OF_ALIGN_VERT_TOP) {
								in.y = 0;
							}
							else if (job.exportSettings.vertAlignment == OF_ALIGN_VERT_BOTTOM) {
								in.y *= 2;
							}
							job.fbos[i]->draw(in);
							break;
						default:
							break;
						}
					
						outFbo.end();
						job.fbos[i] = shared_ptr<ofFbo>(new ofFbo(outFbo));
					}

					// read to of Image
					ofPixels p;
					job.fbos[i]->readToPixels(p);
					ofImage img;
					img.setFromPixels(p);

					// decide if local folder or web upload
					if (ofIsStringInString(path, "http://") || ofIsStringInString(path, "https://")) {
						// save local
						img.save("temp/" + filename, job.exportSettings.quality);
						// upload to server
						imageUploader.sendRequest("temp/" + filename);
					}
					else {
						img.save(exportPath, job.exportSettings.quality);
					}
				}
				


			//}));
		}
	}

	void ofxModulePictureExporter::exportMp4(ImageExportJob job) {
		// first export image sequence
		auto tJob = job;
		tJob.exportSettings.outputType = "png";
		tJob.exportPaths.push_back("");
		tJob.filename = "tExport_[nnnn]";
		exportImages(tJob);

		// combine images to mp4
		string ffmpeg = ofFilePath::getAbsolutePath(FFMPEG_FOLDER);

		for (auto& path : job.exportPaths) {
			string loopCmd = "";
			if (job.exportSettings.loop == OF_LOOP_NORMAL) {
				loopCmd = "-filter_complex loop=loop=" + ofToString(job.exportSettings.nLoops) +":size=" + ofToString(job.totalFrames) + " ";
				loopCmd += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB "; //avoid frame drop
			} else if (job.exportSettings.loop == OF_LOOP_PALINDROME) {
				loopCmd = "-filter_complex \"[0]reverse[r]; [0][r]concat[1],[1]loop=1:" + ofToString(job.totalFrames *2) ;
				loopCmd += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB \" "; //avoid frame drop
			}

			string convertCmd = ffmpeg; // start ffmpeg 
			convertCmd += " -y "; // overwrite existing file
			convertCmd += " -framerate " + ofToString(job.exportSettings.framerate) + " -i "; // set framerate
			convertCmd += ofFilePath::getAbsolutePath("tExport_%04d.png") + " "; // set image path
			convertCmd += loopCmd; // set loop
			convertCmd += " -c:v libx264 -preset medium ";
			convertCmd += " -pix_fmt yuv420p ";
			convertCmd += ofFilePath::getAbsolutePath(path) + "/" + fileCodeToFilename(job.filename) + "." + job.exportSettings.outputType; //set output path
			cout << convertCmd << endl;
			ofSystem(convertCmd);
		}
		
		// delete temp images
		auto filePaths = fileCodeToFilename(tJob.filename, tJob.images.size());
		string absPath = ofFilePath::getAbsolutePath("");
		for (auto& f:filePaths) {
			string delCmd = "del " + absPath + "\\" + f + ".png";
			ofSystem(delCmd);
		}
		
	}

	void ofxModulePictureExporter::readMetaData(ImageExportJob & job, ofJson json)
	{
		if (json["metaData"] != nullptr) {
			job.metaData = json["metaData"];
		}
	}

	void ofxModulePictureExporter::exportMetaData(ImageExportJob job)
	{
		for (auto& path:job.exportPaths)
		{
			exportMetaData(path +"/"+job.filename + ".json", job.metaData);
		}
	}

	void ofxModulePictureExporter::exportMetaData(string path, ofJson json)
	{
		if (json != nullptr && json.size() > 0) {
			ofSavePrettyJson(path, json);
		}
	}

	vector<string> ofxModulePictureExporter::createFfmpegCommand(ImageExportJob job) {

		vector<string> out;
		// combine images to mp4
		string ffmpeg = ofFilePath::getAbsolutePath(FFMPEG_FOLDER);

		for (auto& path : job.exportPaths) {
			string filterCommand = "";
			// only 1 input layer
			if (job.movies.size() == 1) {
				if (job.exportSettings.loop == OF_LOOP_NORMAL) {
					filterCommand = "-filter_complex \"loop=loop=" + ofToString(job.exportSettings.nLoops) + ":size=" + ofToString(job.totalFrames) + " ";
					filterCommand += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB \" "; //avoid frame drop
				} else if (job.exportSettings.loop == OF_LOOP_PALINDROME) {
					filterCommand = "-filter_complex \"[0]reverse[r]; [0][r]concat[1],[1]loop=1:" + ofToString(job.totalFrames * 2);
					filterCommand += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB \" "; //avoid frame drop
				}
			} // multiple input layers 
			else {
				string filterBase = "-filter_complex \"";

				for (size_t i = 0; i < job.movies.size()-1; i++){
					filterBase += "overlay=0:0 ";
					if (i < job.movies.size() - 2) {
						filterBase += ",";
					}
				}
				if (job.exportSettings.loop == OF_LOOP_NONE) {
					filterCommand = filterBase + "\"";
				}
				else if (job.exportSettings.loop == OF_LOOP_NORMAL) {
					filterCommand = filterBase + "[o]; [o]loop=loop=" + ofToString(job.exportSettings.nLoops) + ":size=" + ofToString(job.totalFrames) + " ";
					filterCommand += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB \" "; //avoid frame drop
				} else if (job.exportSettings.loop == OF_LOOP_PALINDROME) {
					// TODO: enable multiple repetations
					filterCommand = filterBase + "[o];[o]split[o1][o2]; [o1]reverse[r]; [o2][r]concat "; // [1], [1]loop = 1:" + ofToString(job.totalFrames * 2);
					filterCommand += ",setpts=N/" + ofToString(job.exportSettings.framerate) + "/TB \" "; //avoid frame drop
				}
			}

			string convertCmd = ffmpeg; // start ffmpeg 
			convertCmd += " -y "; // overwrite existing file
			//convertCmd += " -framerate " + ofToString(path.second.framerate); // set 
			for (auto& file:job.movies) {
				convertCmd += " -i " + ofFilePath::getAbsolutePath(file) + " "; // set image path
			}
			convertCmd += filterCommand; // set filter
			convertCmd += " -c:v libx264 -preset medium ";
			convertCmd += " -pix_fmt yuv420p ";
			convertCmd += ofFilePath::getAbsolutePath(path) + "/" + fileCodeToFilename(job.filename) + "." + job.exportSettings.outputType; //set output path
			//cout << convertCmd << endl;
			//ofSystem(convertCmd);
			out.push_back(convertCmd);
		}
		return out;
	}

	vector<string> ofxModulePictureExporter::matchesInRegex(string _str, string _reg) {
		regex regEx(_reg, regex_constants::icase);
		vector<string> results;
		auto wordsBegin = sregex_iterator(_str.begin(), _str.end(), regEx);
		auto wordsEnd = sregex_iterator();

		for (std::sregex_iterator i = wordsBegin; i != wordsEnd; ++i) {
			smatch m = *i;
			results.push_back(m.str());
		}

		return results;
	}

	ExportSettings ofxModulePictureExporter::readExportSettings(ofJson json) {
		ExportSettings ret;
		updateExportSettings(ret, json);
		return ret;
	}

	void ofxModulePictureExporter::readPathAndStyleSettings(ImageExportJob & job, ofJson config) {
		// get export paths
		if (!config["paths"].is_null()) {
			if (config["paths"].is_array()) {
				if (config["paths"].size() > 1) {
					job.exportPaths = config["paths"].get<vector<string>>();
				}
				else {
					job.exportPaths.push_back(config["paths"].get<string>());
				}
			}
			else {
				job.exportPaths.push_back(config["paths"].get<string>());
			}
		}

		//get export style
		job.exportSettings = styles[defaultStyle];
		if (!config["style"].is_null()) {
			if (styles.find(config["style"]) != styles.end()) {
				job.exportSettings = styles[config["style"].get<string>()];
			}
		}
		else if (!config["styleSettings"].is_null()) {
			updateExportSettings(job.exportSettings, config["styleSettings"]);
		}

		if (!config["loopType"].is_null() && config["loopType"].get<string>() != "") {
			if (config["loopType"].get<string>() == "none") {
				job.exportSettings.loop = OF_LOOP_NONE;
			}
			else if (config["loopType"].get<string>() == "loop") {
				job.exportSettings.loop = OF_LOOP_NORMAL;
			}
			else if (config["loopType"].get<string>() == "palindrome") {
				job.exportSettings.loop = OF_LOOP_PALINDROME;
			}
		}

		// get filename
		if (config["filename"].is_null()) {
			job.filename = "[yy]-[mm]-[dd]-[s]";
			if (job.inputType == IMAGE && job.images.size() > 0) job.filename += "_[n]";
			if (job.inputType == TEXTURE && job.textures.size() > 0) job.filename += "_[n]";
			if (job.inputType == FBO && job.fbos.size() > 0) job.filename += "_[n]";
		} else {
			job.filename = config["filename"].get<string>();
		}
	}

	void ofxModulePictureExporter::updateExportSettings(ExportSettings & settings, ofJson json) {
		if (!json["outputType"].is_null() && json["outputType"].get<string>() != "") {
			settings.outputType = json["outputType"].get<string>();
		}
		if (!json["width"].is_null()) {
			settings.width = json["width"];
		}
		if (!json["height"].is_null()) {
			settings.height = json["height"];
		}
		if (!json["framerate"].is_null()) {
			settings.framerate = json["framerate"];
		}
		if (!json["fit"].is_null()) {
			string fit = json["fit"].get<string>();
			if (fit == "fill") {
				settings.fit = OF_SCALEMODE_FILL;
			}else if (fit == "fit") {
				settings.fit = OF_SCALEMODE_FIT;
			}else if (fit == "stretch") {
				settings.fit = OF_SCALEMODE_STRETCH_TO_FILL;
			}else if (fit == "center") {
				settings.fit = OF_SCALEMODE_CENTER;
			}
		}

		if (!json["quality"].is_null()) {
			string quality = json["quality"].get<string>();
			if (quality == "best") {
				settings.quality = OF_IMAGE_QUALITY_BEST;
			}
			else if (quality == "high") {
				settings.quality = OF_IMAGE_QUALITY_HIGH;
			}
			else if (quality == "medium") {
				settings.quality = OF_IMAGE_QUALITY_MEDIUM;
			}
			else if (quality == "low") {
				settings.quality = OF_IMAGE_QUALITY_LOW;
			}
			else if (quality == "worst") {
				settings.quality = OF_IMAGE_QUALITY_WORST;
			}
		}
		if (!json["loopType"].is_null() && json["loopType"].get<string>() != "") {
			if (json["loopType"].get<string>() == "none") {
				settings.loop = OF_LOOP_NONE;
			} else if (json["loopType"].get<string>() == "loop") {
				settings.loop = OF_LOOP_NORMAL;
			} else if (json["loopType"].get<string>() == "palindrome") {
				settings.loop = OF_LOOP_PALINDROME;
			}
		}
		if (!json["nLoops"].is_null()) {
			settings.nLoops = json["nLoops"];
		}

		if (!json["vertAlignment"].is_null() && json["vertAlignment"].get<string>() != "") {
			if (json["vertAlignment"].get<string>() == "bottom") {
				settings.vertAlignment = OF_ALIGN_VERT_BOTTOM;
			}
			else if (json["vertAlignment"].get<string>() == "center") {
				settings.vertAlignment = OF_ALIGN_VERT_CENTER;
			}
			else if (json["vertAlignment"].get<string>() == "top") {
				settings.vertAlignment = OF_ALIGN_VERT_TOP;
			}
		}

		if (!json["horzAlignment"].is_null() && json["horzAlignment"].get<string>() != "") {
			if (json["horzAlignment"].get<string>() == "left") {
				settings.horzAlignment = OF_ALIGN_HORZ_LEFT;
			}
			else if (json["horzAlignment"].get<string>() == "center") {
				settings.horzAlignment = OF_ALIGN_HORZ_CENTER;
			}
			else if (json["horzAlignment"].get<string>() == "right") {
				settings.horzAlignment = OF_ALIGN_HORZ_RIGHT;
			}
		}
	}

	void ofxModulePictureExporter::setupImageUploader(PostRequestFileSettings settings)
	{
		imageUploader.setup(settings);
	}

	void ofxModulePictureExporter::startVideoCapture(ofxTextureRecorder::VideoSettings vidSettings, int recorderId) {
		// find unused recorder
		bool recorderFound = false;
		int recId = 0;
		for (auto& recorder:imageSaver) {
			if (!recorderFound && !isVideoCapturing[recorder.first]) {
				recorderFound = true;
				recId = recorder.first;
			}
		}
		if (recorderFound) {
			auto it = imageSaver.find(recId);
			if (it != imageSaver.end()) {
				// Swap value from oldKey to newKey, note that a default constructed value 
				// is created by operator[] if 'm' does not contain newKey.
				std::swap(imageSaver[recorderId], it->second);
				// Erase old key-value from map
				imageSaver.erase(it);
			}

			isVideoCapturing.erase(recorderId);
		}
		// or create a new one
		else {
			imageSaver[recorderId] = shared_ptr<ofxTextureRecorder>(new ofxTextureRecorder());
		}

		
		imageSaver[recorderId]->setup(vidSettings);
		isVideoCapturing[recorderId] = true;
		ofLogNotice("ofxModulePictureExporter", "start video capture");
	}

	void ofxModulePictureExporter::addVideoFrame(ofTexture frame, int recorderId) {
		if (isVideoCapturing.find(recorderId) != isVideoCapturing.end() && isVideoCapturing[recorderId]) {
			imageSaver[recorderId]->save(frame);
		} else {
			ofLogError("ofxModulePictureExporter", "start video capture before adding frames");
		}
	}

	void ofxModulePictureExporter::stopVideoCapture(int recorderId) {
		if (isVideoCapturing.find(recorderId) != isVideoCapturing.end() && isVideoCapturing[recorderId]) {
			imageSaver[recorderId]->stop();
		}
	}

	void ofxModulePictureExporter::copyFile(string file, vector<string> dest) {
		for (auto& d:dest) {
			copyFile(file, d);
		}
	}

	void ofxModulePictureExporter::copyFile(string file, string dest) {
		string cmd = "copy " + file + " " + dest;
		ofSystem(cmd);
	}
	void ofxModulePictureExporter::onImageUploaded(ofJson& event)
	{
		if (event["status"].get<int>() == 200) {
			event["success"] = true;
		}
		else {
			event["success"] = false;
		}

		notifyEvent("imageUploaded", event);
	}
	/*
	void ImageExportThread::threadedFunction()
	{
		while (isThreadRunning()) {
			if (containers.size() > 0) {

				for (int i = 0; i < containers.front().job.images.size(); ++i) {
					string filename = containers.front().filePaths.front()[i] + ".";
					filename += containers.front().job.exportSettings.outputType;

					string exportPath;
					exportPath = containers.front().path + "/" + filename;

					int width = containers.front().job.images.front()->getWidth();
					int height = containers.front().job.images.front()->getHeight();

					// resize and crop
					if (containers.front().job.images[i]->getWidth() != width || containers.front().job.images[i]->getHeight() != height) {

						ofRectangle in{ 0,0,containers.front().job.images[i]->getWidth(),containers.front().job.images[i]->getHeight() };
						ofRectangle out{ 0,0,float(width),float(height) };

						in.scaleTo(out, containers.front().job.exportSettings.fit);

						containers.front().job.images[i]->resize(in.width, in.height);
						containers.front().job.images[i]->crop(0.5 * (in.width - out.width), 0.5 * (in.height - out.height), out.width, out.height);
					}
					containers.front().job.images[i]->save(exportPath, containers.front().job.exportSettings.quality);
				}
				std::unique_lock<std::mutex> lock(mutex);
				containers.pop_front();
				condition.notify_all();
			}
			else {
				sleep(20);
			}
			
		}
	}
	void ImageExportThread::addExport(ExportContainer container)
	{
		std::unique_lock<std::mutex> lock(mutex);
		containers.push_back(container);
		condition.notify_all();
	}*/
}
