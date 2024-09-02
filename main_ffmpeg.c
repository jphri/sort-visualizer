#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <cairo.h>

#define WIDTH  800
#define HEIGHT 600

#define FPS         (1.0/60.0)
#define AUDIO_FREQ  (1.0/44100.0)
#define BUFFER_SIZE (FPS/AUDIO_FREQ)

typedef int32_t i32;

static double get_time();

static float make_audio_wave(float time, float freq);

static void make_audio(int v1, int v2);
static void make_frame(int v1, int v2);
static void render(int v1, int v2);
static void write_frame();
static void write_audio();
static void check_position(int i);
static void wait(i32 time);

static void shuffle_array();
static void check_array();
static void bubble_sort();
static void merge_sort();
static void quick_sort();
static void radix_sort_4();
static void radix_sort_8();
static void radix_sort_16();
static void insert_sort();
static void bogo_sort();

static void merge_sub_sort(i32 start, i32 end);
static void quick_sub_sort(i32 start, i32 end);
static void max_heapify(i32 idx, i32 arrsize);

/* operations */
static i32  compare(i32 a, i32 b);
static void swap(i32 arr_pos, i32 new_arr_pos);
static void write_arr(i32 arr_pos, i32 v);
static void write_sub(i32 arr_pos, i32 v);
static i32  read_sub(i32 arr_pos);
static i32  read_arr(i32 arr_pos);
static void wait_frames(i32 time);

static i32 array_size = (4096);
static i32 checked_array;
static i32 array_access;
static i32 skip_frames;
static int comparisons;
static i32 *array;
static i32 *sub_array;
static float target_framerate;
static double prev_time;
static double exit_time, process_time;
static const char *sort_name;

static i32 width, height;
static cairo_surface_t *surface;
static cairo_t *cairo;

static short audio_buffer[(int)BUFFER_SIZE];
static float old_audio_p1 = 0;

int
main()
{
	array = malloc(sizeof(i32) * array_size);
	if(!array) {
		perror("malloc()");
		exit(EXIT_FAILURE);
	}

	sub_array = malloc(sizeof(i32) * array_size);
	if(!sub_array) {
		perror("malloc()");
		exit(EXIT_FAILURE);
	}

	for(i32 i = 0; i < array_size; i++) {
		array[i] = i;
		sub_array[i] = 0;
	}

	width = WIDTH;
	height = HEIGHT;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo = cairo_create(surface);
	cairo_select_font_face(cairo, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	prev_time = get_time();

	#define DO_IT(FN) \
		shuffle_array(); \
		wait_frames(60); \
		exit_time = get_time(); \
		FN(); \
		check_array(); \
		wait_frames(60);

	skip_frames = 64;
	DO_IT(radix_sort_4);
	DO_IT(radix_sort_8);
	DO_IT(radix_sort_16);
	DO_IT(quick_sort);
	DO_IT(merge_sort);
	array_size = 256;
	skip_frames = 8;
	DO_IT(bubble_sort);
	DO_IT(insert_sort);
	
	free(array);
	free(sub_array);

	return 0;
}

static void
shuffle_array()
{
	array_access = 0;
	comparisons = 0;
	sort_name = "Shuffling...";
	for(i32 i = 0; i < array_size - 1; i++) {
		i32 r = array_size - i;
		i32 rr = random() % r;
		
		swap(i, rr + i);
	}
	array_access = 0;
	comparisons = 0;
	process_time = 0;
}

static void
check_array()
{
	int old_skip = skip_frames;
	for(; checked_array < array_size; checked_array++) {
		if(array[checked_array] != checked_array) {
			printf("%s failed...\n", sort_name);
			exit(EXIT_FAILURE);
		}
		render(checked_array, -1);
	}
	skip_frames = 0;
	render(-1, -1);
	checked_array = 0;
	skip_frames = old_skip;
}

void
bubble_sort()
{
	array_access = 0;
	process_time = 0;
	comparisons = 0;
	sort_name = "Bubble Sort";
	for(i32 k = 0; k < array_size; k++)
	for(i32 i = 0; i < array_size - 1; i++) {
		if(compare(i, i + 1) > 0) {
			swap(i + 1, i);
		}
	}
}

void
insert_sort()
{
	array_access = 0;
	comparisons = 0;
	process_time = 0;
	sort_name = "Insert Sort";
	for(i32 k = 0; k < array_size; k++)
	for(i32 i = k; i > 0; i--) {
		if(compare(i, i - 1) < 0)
			swap(i, i - 1);
		else
			break;
	}
}

void
merge_sort()
{
	array_access = 0;
	comparisons = 0;
	process_time = 0;
	sort_name = "Merge Sort";
	merge_sub_sort(0, array_size);
}

void
quick_sort()
{
	array_access = 0;
	comparisons = 0;
	process_time = 0;
	sort_name = "Quick Sort";
	quick_sub_sort(0, array_size);
}

void
radix_sort_4()
{
	int buckets[4] = {0};
	array_access = 0;
	comparisons = 0;
	process_time = 0;

	sort_name = "Radix Sort (LSD, base-4)";

	/* 6 = log2(array_size) / 2 */
	for(int k = 0; k < 6; k++) {
		for(int i = 0; i < 4; i++) {
			buckets[i] = (i * array_size) / 4;
		}

		for(int i = 0; i < array_size; i++) {
			i32 value = read_arr(i);
			i32 buck = (value >> (k * 2)) % 4;
			sub_array[buckets[buck]++] = value;
		}

		for(int i = 0, arr_pos = 0; i < 4; i++) {
			int bucket_base = (i * array_size) / 4;
			int bucket_length = buckets[i] - bucket_base;
			
			for(int j = 0; j < bucket_length; j++) {
				write_arr(arr_pos++, sub_array[bucket_base + j]);
			}
		}
	}
}

void
radix_sort_16()
{
	int buckets[16] = {0};
	array_access = 0;
	comparisons = 0;
	process_time = 0;

	sort_name = "Radix Sort (LSD, base-16)";

	/* 6 = log2(array_size) / 2 */
	for(int k = 0; k < 3; k++) {
		for(int i = 0; i < 16; i++) {
			buckets[i] = (i * array_size) / 16;
		}

		for(int i = 0; i < array_size; i++) {
			i32 value = read_arr(i);
			i32 buck = (value >> (k * 4)) % 16;
			sub_array[buckets[buck]++] = value;
		}

		for(int i = 0, arr_pos = 0; i < 16; i++) {
			int bucket_base = (i * array_size) / 16;
			int bucket_length = buckets[i] - bucket_base;
			
			for(int j = 0; j < bucket_length; j++) {
				write_arr(arr_pos++, sub_array[bucket_base + j]);
			}
		}
	}
}
void
radix_sort_8()
{
	int buckets[8] = {0};
	array_access = 0;
	comparisons = 0;
	process_time = 0;

	sort_name = "Radix Sort (LSD, base-8)";

	/* 6 = log2(array_size) / 2 */
	for(int k = 0; k < 4; k++) {
		for(int i = 0; i < 8; i++) {
			buckets[i] = (i * array_size) / 8;
		}

		for(int i = 0; i < array_size; i++) {
			i32 value = read_arr(i);
			i32 buck = (value >> (k * 3)) % 8;
			sub_array[buckets[buck]++] = value;
		}

		for(int i = 0, arr_pos = 0; i < 8; i++) {
			int bucket_base = (i * array_size) / 8;
			int bucket_length = buckets[i] - bucket_base;
			
			for(int j = 0; j < bucket_length; j++) {
				write_arr(arr_pos++, sub_array[bucket_base + j]);
			}
		}
	}
}

void
bogo_sort()
{
	array_access = 0;
	comparisons = 0;
	for(;;) {
		for(i32 i = 0; i < array_size - 1; i++) {
			if(compare(i + 1, i) < 0) {
				shuffle_array();
				goto not_yet;
			}
		}
		break;
	not_yet:
		continue;
	}
}

void
merge_sub_sort(i32 start, i32 end)
{
	static i32 last_start, last_end;

	i32 i, j, k;
	if(end - start <= 1)
		return;

	assert(last_start != start || last_end != end);
	last_start = start;
	last_end = end;
	
	i32 med = start + (end - start) / 2;

	merge_sub_sort(start, med);
	merge_sub_sort(med, end);
	
	i = start;
	j = med;
	k = 0;

	while(i < med && j < end) {
		if(compare(i, j) < 0) {
			sub_array[k++] = read_arr(i++);
		} else {
			sub_array[k++] = read_arr(j++);
		}
	}
	
	while(i < med)
		sub_array[k++] = array[i++];

	while(j < end)
		sub_array[k++] = array[j++];

	for(i32 l = 0; l < k; l++) {
		write_arr(l + start, sub_array[l]);
	}
}

void
quick_sub_sort(i32 start, i32 end)
{
	i32 length = end - start;
	if(length <= 1)
		return;

	i32 i = start - 1, j = start;

	while(j < end) {
		if(compare(j, end - 1) <= 0) {
			i32 tmp;
			i++;

			swap(i, j);
		}
		j++;
	}
	
	quick_sub_sort(start, i);
	quick_sub_sort(i, end);
}

void
make_frame(int p1, int p2)
{
	char buffer[1024];
	int bar_height = height - 100;

	cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);

	for(i32 i = 0; i < array_size; i++) {
		i32 bh = (array[i] * bar_height) / array_size;
		double r, g, b;
		if(i == p1 || i == p2) {
			r = 1.0;
			g = 0.0;
			b = 0.0;
		} else {
			if(i < checked_array) {
				r = 0.0;
				g = 1.0;
				b = 0.0;
			} else {
				r = 1.0;
				g = 1.0;
				b = 1.0;
			}
		}

		cairo_set_source_rgba(cairo, r, g, b, 0.5);
		cairo_move_to(cairo, (i * width) / array_size, height);
		cairo_line_to(cairo, (i * width) / array_size, height - bh);
		cairo_stroke(cairo);
	}

	cairo_set_source_rgba(cairo, 1.0, 1.0, 0.0, 1.0);
	cairo_set_font_size(cairo, 16);

	cairo_move_to(cairo, 15, 15);
	snprintf(buffer, sizeof(buffer), 
			"Sort name: %s",
			sort_name ? sort_name : "(no name)",
			process_time * 1000);
	cairo_show_text(cairo, buffer);
	cairo_fill(cairo);

	cairo_move_to(cairo, 15, 15 + 16 * 1);
	snprintf(buffer, sizeof(buffer), 
			"Array size: %d",
			array_size);
	cairo_show_text(cairo, buffer);
	cairo_fill(cairo);

	cairo_move_to(cairo, 15, 15 + 16 * 2);
	snprintf(buffer, sizeof(buffer), 
			"Array Access: %d",
			array_access);
	cairo_show_text(cairo, buffer);
	cairo_fill(cairo);

	cairo_move_to(cairo, 15, 15 + 16 * 3);
	snprintf(buffer, sizeof(buffer), 
			"Comparisons: %d",
			comparisons);
	cairo_show_text(cairo, buffer);
	cairo_fill(cairo);

	cairo_move_to(cairo, 15, 15 + 16 * 4);
	snprintf(buffer, sizeof(buffer), 
			"Process Time: %f ms", 
			process_time * 1000);
	cairo_show_text(cairo, buffer);
	cairo_fill(cairo);
}

void
make_audio(int p1, int p2)
{
	(void)p2;	
	static float phase_accum = 0;
	float freq_scale = (float)array[p1] / array_size;

	for(int i = 0; i < BUFFER_SIZE; i++) {
		const float new_wave = make_audio_wave(phase_accum, freq_scale);
		const float old_wave = make_audio_wave(phase_accum, old_audio_p1);
		
		float wave = new_wave * (i / BUFFER_SIZE) + old_wave * (1 - i / BUFFER_SIZE);
		audio_buffer[i] = wave * 32767 / 8;
		phase_accum += AUDIO_FREQ;
	}
	old_audio_p1 = freq_scale;
}

void
render(int p1, int p2)
{
	double current_time = get_time();
	double delta = current_time - prev_time;
	double delta_process = current_time - exit_time;
	prev_time = current_time;
	static int accumulator;

	process_time += delta_process;

	if(++accumulator > skip_frames) {
		make_frame(p1, p2);
		make_audio(p1, p2);
		write_frame();
		write_audio();
		accumulator = 0;
	}
	exit_time = get_time();
}

void
write_frame()
{
	cairo_surface_flush(surface);
	uint32_t *data = cairo_image_surface_get_data(surface);
	write(3, data, sizeof(data[0]) * width * height);
}

void
write_audio()
{
	write(4, audio_buffer, sizeof(audio_buffer[0]) * BUFFER_SIZE);
}

i32
compare(i32 a, i32 b)
{
	array_access += 2;
	comparisons += 1;
	return array[a] - array[b];
}

void
swap(i32 v1, i32 v2)
{
	i32 tmp;
	tmp = array[v1];
	array[v1] = array[v2];
	array[v2] = tmp;

	array_access += 2;

	render(v1, v2);
}

void
write_arr(i32 position, i32 value)
{
	array_access += 1;
	array[position] = value;
	render(position, -1);
}

i32
read_arr(i32 position)
{
	i32 r = array[position];
	array_access += 1;
	render(position, -1);
	return r;
}

void
wait_frames(i32 time)
{
	cairo_surface_flush(surface);
	memset(audio_buffer, 0, sizeof(audio_buffer));
	for(int i = 0; i < time; i++) {
		write_frame();
		write_audio();
	}
}

double
get_time()
{
	struct timespec tm;
	clock_gettime(CLOCK_REALTIME, &tm);
	return tm.tv_sec + tm.tv_nsec / 1000000000.0;
}

float
make_audio_wave(float time, float freq_scale)
{
	const float freq = 220 + freq_scale * 780;
	const float phase = 2 * 3.1415926535 * freq * time;
	return sinf(phase) + sinf(phase * 2.0) / 2.0 + sinf(phase * 4.0) / 4.0;
}
