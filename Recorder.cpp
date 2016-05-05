#include "Recorder.hpp"
#include "ColorMap.hpp"

Recorder::Recorder(){
	FrameTime = 0;
}

void Recorder::run(){
	f1 = fopen("ColorStream", "ab");
		
	if(!f1)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier\n");
		exit(1);
	}
	
	f2 = fopen("DepthStream", "ab");
		
	if(!f2)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier\n");
		exit(1);
	}

	while(traitement) loop();
	
	uint8_t endcode[] = {0, 0, 1, 0xb7};
	fwrite(endcode, 1, sizeof(endcode), f1);
	fclose(f1);
	fwrite(endcode, 1, sizeof(endcode), f2);
	fclose(f2);
}

bool Recorder::loop(){
	AVFrame* ColorFrame, DepthFrame;
	DepthFrame = av_frame_alloc();
	Colorframe = av_frame_alloc();
	if(!ColorFrame)
	{
		fprintf(stderr, "Impossible d'allouer la frame\n");
		exit(1);
	}
	if(!DepthFrame)
	{
		fprintf(stderr, "Impossible d'allouer la frame\n");
		exit(1);
	}
	bool done = false;
	CurrentFrames = getPointCloud();
	
	for(int i = 0; i < CurrentFrames.size(); i++){
		//Traitement de chaque frame
		PointCloud::shared_ptr PC = CurrentFrames[i];
		ColorMap color, depth;
		color = PC->getColorMap();
		depth = PC->getDepthMap();
		if(!done){
			done = true;
			ColorFrame->height = color->height;
			DepthFrame->height = depth->height;
		}
		uint8_t* data;
		AVCodec* codec;
		AVCodecContext* context;
		
		AVPacket pkt;
		
		codec = avcodec_find_encoder(AV_CODEC_ID_NONE);
		if(!codec){
			fprintf(stderr, "Le Codec est introuvable\n");
			exit(1);
		}
		context = avcodec_alloc_context3(codec);
		if(!context1)
		{
			fprintf(stderr, "Le contexte de Codec n'a pas pu etre alloue\n");
			exit(1);
		}
		//Vitesse en bit par seconde de l'enregistrement
		context->bit_rate = 400000;
		context->time_base = (AVRational){1,25};
		context->gop_size = 10;
		context->max_b_frames = 1;
		context->width = color.width;
		context->height = color.height;
		
		//La partie ouverture, allocation de la frame et ecriture des données
		//pourra etre écrit dans une même fonction
		
		//Ouverture des fichiers et codecs
		
		int PixelSize = color->size/(color->height * color->width);
		
		
		switch(PixelSize)
		{
			case 24:
				context->pix_fmt = AV_PIX_FMT_RGB_24;
				break;
			case 16:
				context->pix_fmt = AV_PIX_FMT_YUYV422;
				break;
		}
		
		if(avcodec_open2(context, codec, NULL) < 0)
		{
			fprintf(stderr, "Impossible d'ouvrir le Codec\n");
			exit(1);
		}
	
		ColorFrame->format = context->pix_fmt;
		ColorFrame->width = context->width;
	
		int ret = av_image_alloc(ColorFrame->data, ColorFrame->linesize, ColorFrame->width, ColorFrame->height, 				frame->format, 32);
		if(ret < 0)
		{
			fprintf(stderr, "Impossible d'allour le raw image buffer");
			exit(1);
		}		
		
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
	
		//Gestion de la frame
		switch(PixelSize)
		{
			case 24:
				data = (uint8_t*) color.data;
				for(int j = 0; j < context->height; j++)
				{
					for(int k = 0; k < context->width; k++)
					{
						ColorFrame.data[0][j * ColorFrame->linesize[0] + k + i*color.size] = data[3*j];
						ColorFrame.data[1][j * ColorFrame->linesize[1] + k + i*color.size] = data[3*j + 1];
						ColorFrame.data[2][j * ColorFrame->linesize[2] + k + i*color.size] = data[3*j + 2];
					}
				}
				break;
			case 16:
				//Ecrire des données dans frame.data sous fomre YUYV
				data = (uint8_t) color.data;
				for(int j = 0; j < context->height; j++)
				{
					for(int k = 0; k < context->width; k++)
					{
						if(k%2 == 0)
						{
							ColorFrame.data[0][j*ColorFrame->linesize[0] + k + i*color.size] = data[4*j];
						}
						else
						{
							ColorFrame.data[0][j*ColorFrame->linesize[0]+k + i*color.size] = data[4*j+2];
						}
						ColorFrame.data[1][j*ColorFrame->linesize[1] + k + i*color.size] = data[4*j+1];
						ColorFrame.data[2][j*ColorFrame->linesize[2] + k + i*color.size] = data[4*j+3];
					}
				}
				break;
		}
		ColorFrame->pts = FrameTime;
		context->width = depth.width;
		context->height = depth.height;
		
		PixelSize = depth->size/(depth->height * depth->width);
		
		switch(PixelSize)
		{
			case 16:
				//Encode Gray 16
				context->pix_fmt = AV_PIX_FMT_GRAY16;
				break;
			case 8:
				//Encode Gray 8
				context->pix_fmt = AV_PIX_FMT_GRAY8;
				break;
		}
		
		if(avcodec_open2(context, codec, NULL) < 0)
		{
			fprintf(stderr, "Impossible d'ouvrir le Codec\n");
			exit(1);
		}
	
		DepthFrame->format = context->pix_fmt;
		//Pour l'instant, je pars du principe que ça change pas
		//frame->width = context->width;
		//frame->height = context->height;
	
		ret = av_image_alloc(DepthFrame->data, DepthFrame->linesize, DepthFrame->width, DepthFrame->height, DepthFrame->format, 32);
		if(ret < 0)
		{
			fprintf(stderr, "Impossible d'allour le raw image buffer");
			exit(1);
		}		
		
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		
		switch(PixelSize)
		{
			case 16:
				data = (uint8_t*) depth.data;
				for(int j = 0; j < context->height; j++)
				{
					for(int k = 0; k < context->width; k++)
					{
						DepthFrame.data[0][j * DepthFrame->linesize[0] + k] = data[2*j];
						DepthFrame.data[1][j * DepthFrame->linesize[1] + k] = data[2*j + 1];
					}
				}
				break;
			case 8:
				data = (uint8_t*) depth.data;
				for(int j = 0; j < context->height; j++)
				{
					for(int k = 0; k < context->width; k++)
					{
						DepthFrame.data[0][j * DepthFrame->linesize[0] + k] = data[j];
					}
				}
				break;
		}
		
		DepthFrame->pts = FrameTime;
		
		avcodec_close(context);
	
		av_free(context);
		
		free(data);
	}
	
	//Ecriture dans le fichier
		int output = 0;
		int ret = avcodec_encode_video2(context, &pkt, ColorFrame, &output);
		if(ret<0)
		{
			fprintf(stderr, "Erreur d'encodage de la frame\n");
			exit(1);
		}
		if(output)
		{
			printf("Ecriture de la frame");
			fwrite(pkt.data, 1, pkt.size, f1);
		}
		
		output = 0;
		ret = avcodec_encode_video2(context, &pkt, DepthFrame, &output);
		if(ret<0)
		{
			fprintf(stderr, "Erreur d'encodage de la frame\n");
			exit(1);
		}
		if(output)
		{
			printf("Ecriture de la frame");
			fwrite(pkt.data, 1, pkt.size, f2);
		}
	av_free_packet(&pkt);
	av_freep(&frame->data[0]);
		av_frame_free(&frame);
	FrameTime++;
	
	
	
	return true;
}
