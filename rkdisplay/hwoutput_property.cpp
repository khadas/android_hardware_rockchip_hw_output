/*
 * DRM based mode setting test program
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * This fairly simple test program dumps output in a similar format to the
 * "xrandr" tool everyone knows & loves.  It's necessarily slightly different
 * since the kernel separates outputs into encoder and connector structures,
 * each with their own unique ID.  The program also allows test testing of the
 * memory management and mode setting APIs by allowing the user to specify a
 * connector and mode to use for mode setting.  If all works as expected, a
 * blue background should be painted on the monitor attached to the specified
 * connector after the selected mode is set.
 *
 * TODO: use cairo to write the mode info on the selected output once
 *       the mode has been programmed, along with possible test patterns.
 */
#include "hwoutput_property.h"

namespace android {

struct post_csc {
	u16 hue;
	u16 saturation;
	u16 contrast;
	u16 brightness;
	u16 r_gain;
	u16 g_gain;
	u16 b_gain;
	u16 r_offset;
	u16 g_offset;
	u16 b_offset;
	u16 csc_enable;
};

struct post_acm {
	s16 delta_lut_h[ACM_DELTA_LUT_H_TOTAL_LENGTH];
	s16 gain_lut_hy[ACM_GAIN_LUT_HY_TOTAL_LENGTH];
	s16 gain_lut_hs[ACM_GAIN_LUT_HS_TOTAL_LENGTH];
	u16 y_gain;
	u16 h_gain;
	u16 s_gain;
	u16 acm_enable;
};

HwOutputProperty::HwOutputProperty(){
}

HwOutputProperty::~HwOutputProperty(){
}

static uint32_t get_property_id(int fd, drmModeObjectProperties *props,
				const char *name)
{
	drmModePropertyPtr property;
	uint32_t i, id = 0;

	/* find property according to the name */
	for (i = 0; i < props->count_props; i++) {
		property = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(property->name, name))
			id = property->prop_id;
		drmModeFreeProperty(property);

		if (id)
			break;
	}

	return id;
}

uint32_t HwOutputProperty::set_csc_info(int fd,  unsigned crtc_id, struct csc_info* info)
{
	unsigned blob_id = 0;
	drmModeObjectProperties *props;
	struct post_csc csc;
	int ret;

	csc.hue = info->hue;
	csc.saturation = info->saturation;
	csc.contrast = info->contrast;
	csc.brightness = info->brightness;
	csc.r_gain = info->r_gain;
	csc.g_gain = info->g_gain;
	csc.b_gain = info->b_gain;
	csc.r_offset = info->r_offset;
	csc.g_offset = info->g_offset;
	csc.b_offset = info->b_offset;
	csc.csc_enable = info->cscEnable;

	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	uint32_t property_id = get_property_id(fd, props, "POST_CSC_DATA");
	if(property_id == 0){
		ALOGE("can't find csc");
		drmModeFreeObjectProperties(props);
		return -1;
	}
	drmModeCreatePropertyBlob(fd, &csc, sizeof(struct post_csc ), &blob_id);
	ret = drmModeObjectSetProperty(fd, crtc_id, DRM_MODE_OBJECT_CRTC, property_id, blob_id);
	drmModeDestroyPropertyBlob(fd, blob_id);
	drmModeFreeObjectProperties(props);

	return 0;
}

uint32_t HwOutputProperty::set_acm_data(int fd, unsigned crtc_id, struct acm_data* data)
{
	unsigned blob_id = 0;
	drmModeObjectProperties *props;
	struct post_acm pacm;
	int ret;

	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	uint32_t property_id = get_property_id(fd, props, "ACM_LUT_DATA");
	if(property_id == 0){
		ALOGE("can't find csc");
		return -1;
	}

	memcpy(pacm.delta_lut_h, data->delta_lut_h, ACM_DELTA_LUT_H_TOTAL_LENGTH);
	memcpy(pacm.gain_lut_hy, data->gain_lut_hy, ACM_GAIN_LUT_HY_TOTAL_LENGTH);
	memcpy(pacm.gain_lut_hs, data->gain_lut_hs, ACM_GAIN_LUT_HS_TOTAL_LENGTH);
	pacm.acm_enable = data->acm_enable;
	pacm.y_gain = data->y_gain;
	pacm.h_gain = data->h_gain;
	pacm.s_gain = data->s_gain;

	drmModeCreatePropertyBlob(fd, &pacm, sizeof(struct post_acm), &blob_id);
	ret = drmModeObjectSetProperty(fd, crtc_id, DRM_MODE_OBJECT_CRTC, property_id, blob_id);
	drmModeDestroyPropertyBlob(fd, blob_id);
	drmModeFreeObjectProperties(props);
	return 0;
}

int HwOutputProperty::set_3x1d_gamma(int fd, unsigned crtc_id, uint32_t size, uint16_t* r, uint16_t* g, uint16_t* b)
{
	unsigned blob_id = 0;
	drmModeObjectProperties *props;
	struct drm_color_lut gamma_lut[size];
	int i, ret;
	for (i = 0; i < size; i++) {
		gamma_lut[i].red = r[i];
		gamma_lut[i].green = g[i];
		gamma_lut[i].blue = b[i];
	}
	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	uint32_t property_id = get_property_id(fd, props, "GAMMA_LUT");
	if(property_id == 0){
		ALOGE("can't find GAMMA_LUT");
	}
	drmModeCreatePropertyBlob(fd, gamma_lut, sizeof(gamma_lut), &blob_id);
	ret = drmModeObjectSetProperty(fd, crtc_id, DRM_MODE_OBJECT_CRTC, property_id, blob_id);
	drmModeDestroyPropertyBlob(fd, blob_id);
	drmModeFreeObjectProperties(props);
	return ret;
}

int HwOutputProperty::set_cubic_lut(int fd, unsigned crtc_id, uint32_t size, uint16_t* r, uint16_t* g, uint16_t* b)
{
	unsigned blob_id = 0;
	drmModeObjectProperties *props;
	struct drm_color_lut cubic_lut[size];
	int i, ret;
	for (i = 0; i < size; i++) {
		cubic_lut[i].red = r[i];
		cubic_lut[i].green = g[i];
		cubic_lut[i].blue = b[i];
	}
	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	uint32_t property_id = get_property_id(fd, props, "CUBIC_LUT");
	if(property_id == 0){
		ALOGE("can't find CUBIC_LUT");
	}
	drmModeCreatePropertyBlob(fd, cubic_lut, sizeof(cubic_lut), &blob_id);
	ret = drmModeObjectSetProperty(fd, crtc_id, DRM_MODE_OBJECT_CRTC, property_id, blob_id);
	drmModeDestroyPropertyBlob(fd, blob_id);
	drmModeFreeObjectProperties(props);
	return ret;
}

}

