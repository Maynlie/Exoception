#ifndef PBOX_RECORDER_H
#define PBOX_RECORDER_H

#include "Display.hpp"
#include "vector.h"

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

class Recorder : public _Display
{
	public:
		Recorder();
		void run();
		bool loop();
	private:
		bool traitement;
		PointCloud::vector CurrentFrames;
		int FrameTime;
		FILE* f1, f2;
};

#endif
