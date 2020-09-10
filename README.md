# ofxModulePictureExporter

Module for [ofxModule](https://github.com/reddoLabs/ofxModule) allows exporting single frames and movies using [ofxTextureRecorder](https://github.com/brinoausrino/ofxTextureRecorder).

Compatible with openFrameworks 0.11.0+

Generate all project files using the openFrameworks Project Generator.

## Required Addons

* [ofxModule](https://github.com/reddoLabs/ofxModule)
* [ofxTextureRecorder (fork by brinoausrino)](https://github.com/brinoausrino/ofxTextureRecorder)
* [ofxVideoRecorder (fork by brinoausrino)](https://github.com/brinoausrino/ofxVideoRecorder)

## Getting started

Have a look at the sample project to get started directly and have a small explanation here.

## Settings

The module can be configured in the settings.json. There you can define export presets. `defaultStyle` sets the default style that will be used. It needs to be defined in styles. `capture` defines the default movie capture settings.

```json
 "PictureExporter": {
        "defaultStyle": "image",
        "styles": [
            {
                "id": "image",
                "outputType": "png"
            },
            {
                "height": 1080,
                "id": "movie",
                "nLoops": 5,
                "outputType": "mp4",
                "width": 1920
            },
            {
                "id": "print",
                "width": 1844,
                "height": 1240,
                "outputType": "png",
                "fit":"fill"
            }
        ],
        "capture":{
            "file":"capture.mov",
            "codec":"hap -format hap_alpha",
            "bitrate":"",
            "framerate":30,
            "extraSettings":""
        }
    }
```

**Style properties**

| property      | type          | description  |
| ------------- |---------------| -----|
| id       | string       | the style id |
| width,height       | int       | image dimensions |
| outputType        | string           | output type (png, jpg ..)|
| fit | string      | fillmode (fit,fill,stretch)|
| nLoops | int      | for movie only : number of repetitions|
| loopMode | string      | for movie only : type of repetition (none,loop,palindrome)|

**Capture properties**

| property      | type          | description  |
| ------------- |---------------| -----|
| file       | string       | the output file |
| codec        | string           | the export codec (see ffmpeg for codec options)|
| bitrate | string      | capture bitrate (leave empty for default)|
| framerate | int      | the desired capture framerate|
| extraSettings | string      | extra ffmpeg settings|


## Export messages

### Export image in default settings

You can export `ofImage`, `ofTexture` or `ofFbo`. Just put the image in the message.

```cpp
// export ofFbo

shared_ptr<ofFbo> fbo = shared_ptr<ofFbo>(new ofFbo());
fbo->allocate(camImage->getWidth(), camImage->getHeight());
fbo->begin();
// draw something
fbo->end();

ofJson msg;
msg["filename"] = "export_ofImage.png";
notifyEvent(fbo,"exportImage", msg);

```

### Export image a specific style settings

```cpp

ofJson msg;
msg["filename"] = "export_ofImage.png";
msg["style"] = "defaultStyle";
notifyEvent(img,"exportImage", msg);

```

### Capture a sequence

#### Start capture

```cpp
ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","start"} }));
e.texture = camImage;
e.destClass = "PictureExporter";
notifyEvent(e);
```

#### Stop capture

```cpp
ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","stop"} }));
e.destClass = "PictureExporter";
notifyEvent(e);
```

#### Add a frame

```cpp
ModuleEvent e = ModuleEvent("ExportControl", "ExportControl", "capture", ofJson({ {"option","frame"} }));
e.destClass = "PictureExporter";
e.texture = camImage;
notifyEvent(e);
```

### Overview of commands

| property      | type          | description  |
| ------------- |---------------| -----|
| filename       | string       | desired output name, you can also use variables for date (see timecodes)|
| style        | string           | the desired export style|
| paths | array of strings      | multiple output paths if desired|

All style and capture properties also work.

#### Timecodes

| code | description |
|------|-------------|
|[y]| year, last 2 digits|
|[yy]|year)|
|[m]|month|
|[mm]|month, leading 0|
|[d]|day|
|[dd]|day, leading 0|
|[h]|hour|
|[hh]|hour leading 0|
|[M]|minutes|
|[MM]|minutes, leading 0|
|[S]|seconds|
|[SS]|seconds, leading 0|
|[i]|milliseconds|