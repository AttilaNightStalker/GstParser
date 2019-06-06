CC=g++
FLAGS=`pkg-config --cflags --libs gstreamer-app-1.0` -lyuv -ljpeg
decoder: main.cpp GstDecoder.cpp GstEncoder.cpp GstCodec.h FrameConverter.cpp FrameConverter.h
	$(CC) -o decoder main.cpp GstDecoder.cpp GstEncoder.cpp FrameConverter.cpp $(FLAGS) -g

clean:
	rm decoder
