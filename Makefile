.PHONY: all clean

all: output.mp4

clean:
	rm -f output.mp4
	rm -f output_no_audio.mp4 audio.aac
	rm -f sort_visualizer

output.mp4: output_no_audio.mp4 audio.aac
	ffmpeg -i output_no_audio.mp4 -i audio.aac -map 0:v -map 1:a -c:a copy -c:v copy $@

output_no_audio.mp4: sort_visualizer
	./sort_visualizer 3>&1 4>/dev/null | ffmpeg -y -f rawvideo -pix_fmt bgra -s 1280x720 -r 60 -i - -c:v libx264 -pix_fmt yuv420p $@

audio.aac: sort_visualizer
	./sort_visualizer 3>/dev/null 4>&1 | ffmpeg -y -f s16le -ar 44100 -ac 1 -i - -b:a 128k $@

sort_visualizer: main_ffmpeg.o
	$(CC) -o $@ $^ `pkg-config --libs cairo` -lm

%.o: %.c
	$(CC) -c -o $@ $< `pkg-config --cflags cairo`

