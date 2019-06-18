# RGA mode or not
RGA = true

ifdef RGA
FLAGS = `pkg-config --cflags --libs gstreamer-app-1.0` -DRGA
else
FLAGS=`pkg-config --cflags --libs gstreamer-app-1.0` -lyuv -ljpeg
endif

CC=g++

decoder: main.cpp GstDecoder.cpp GstEncoder.cpp GstCodec.h FrameConverter.cpp FrameConverter.h
	$(CC) -o decoder main.cpp GstDecoder.cpp GstEncoder.cpp FrameConverter.cpp $(FLAGS) -g

clean:
	rm decoder
