#include "rack.hpp"
#include "dep/dr_wav.h"
#include "dep/dr_flac.h"
#include "dep/dr_mp3.h"

using namespace std;

enum PanningType {SIMPLEPAN, CONSTPOWER };

struct PanPos {
	double left;
	double right;
};

struct AudioParameters {
    AudioParameters();
    float dB;
	PanningType panningType;
    float panningValue;  
    float rackSampleRate;
    bool stop;
    float startRatio,endRatio;
    bool repeat;
    float speed;
};

struct Audio {
    Audio();
    void setParameters(const AudioParameters& );
  	bool loadSample(char *);
    void ejectSong();
    PanPos panning(PanningType, double);
    void processAudioSample();
    void start(void);
    void rewind(void);
    void forward(void);
    bool withinBoundery(void);
    void setPause(bool);
    void setPlay(bool);
    bool getPlay();
    void setStop(bool);
    void setRepeat(bool);
    vector<vector<float>> playBuffer;
    float peak;
    float scaleFac;
    float * pSampleData;
	float samplePos;
    PanningType panningType;
    unsigned int channels;
	unsigned int sampleRate;
	drwav_uint64 totalPCMFrameCount;
    drmp3_config mp3config;
    char * fileName;
    bool pause,loading,fileLoaded,play,stop;
    float left,right;
    float dB;
    float speed;
    float panningValue; 
    float rackSampleRate;
    float beginRatio, endRatio;
    bool up,repeat;
};

