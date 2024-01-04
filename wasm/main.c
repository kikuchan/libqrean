#include <stdio.h>

#include "qrean.h"
#include "image.h"
#include "detector.h"

//char imagebuf[1024 * 100]; // 100kbyte
char imagebuf[1024 * 120]; // 120kbyte
//char imagebuf[1024 * 50]; // 50kbyte
int getIndexImage() {
	return (int)imagebuf;
}

//char inputbuf[1024 * 10]; // 10kbyte
char inputbuf[1024 * 1]; // 1kbyte
int getIndexInput() {
	return (int)inputbuf;
}

char outputbuf[1024];
int getIndexOutput() {
	return (int)outputbuf;
}

image_t* make(int type, int datatype) { // QREAN_CODE_TYPE_QR
	qrean_t qrean = create_qrean(type);
	int res = qrean_write_string(&qrean, inputbuf, datatype);
	if (!res) {
		return 0;
	}
	//qrean_try_write_qr_data(&qrean, inputbuf, datatype);
	size_t width = qrean_get_bitmap_width(&qrean);
	size_t height = qrean_get_bitmap_height(&qrean);
	image_t* img = (image_t*)imagebuf;
	img->buffer = (image_pixel_t*)(void*)(img + sizeof(size_t) * 4);
	img->width = width;
	img->height = height;
	qrean_set_bitmap_color(&qrean, 0x000000FF, 0xFFFFFFFF);
	qrean_read_bitmap(&qrean, img->buffer, width * height * 4, 32);
	return img;
	//image_dump(img, stdout);
}


static void on_found(qrean_detector_perspective_t *warp, void *opaque) {
	qrean_read_string(warp->qrean, outputbuf, sizeof(outputbuf));
}

int detect() {
	double gamma_value = 1.8;

	outputbuf[0] = '\0';
	image_t* img = (image_t*)imagebuf;
	image_t* mono = image_clone(img);

	image_digitize(mono, img, gamma_value);

	int num_candidates = 0;
	qrean_detector_qr_finder_candidate_t* candidates = qrean_detector_scan_qr_finder_pattern(mono, &num_candidates);
	//return num_candidates;
	free(mono);
	
	int found = qrean_detector_try_decode_mqr(mono, candidates, num_candidates, on_found, NULL);
	return found;
}
