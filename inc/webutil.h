/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *          Geunsun Lee <gs86.lee@samsung.com>
 *          Eunyoung Lee <ey928.lee@samsung.com>
 *          Junkyu Han <junkyu.han@samsung.com>
 *          Jeonghoon Park <jh1979.park@samsung.com>
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __POSITION_FINDER_WEBUTIL_H__
#define __POSITION_FINDER_WEBUTIL_H__

#include <stdbool.h>
#include <feature.h>

typedef struct _web_util_sensor_data_s web_util_sensor_data_s;
struct _web_util_sensor_data_s {
	int motion;
	int flame;
	double humidity;
	double temperature;
	bool virbration;
	double co2;
	int soundlevel;
	int tilt;
	int light;
	int collision;
	int obstacle;
	double distance;
	int rain;
	int touch;
	int gas;
	feature_type_e enabled_feature;
	const char *tag;
	const char *hash;
	const char *ip_addr;
};

typedef struct _web_util_image_data_s {
	const char *url;
	const char *path;
} web_util_image_data_s;

int web_util_noti_init(void);
void web_util_noti_fini(void);
int web_util_noti_post_image_data(web_util_image_data_s *post_data, const char *tag,
	const void *image_data, unsigned int image_size);
int web_util_noti_post(const char *resource, const char *json_data);

/* A newly allocated string is assigned to @res, which ought to be freed after use*/
int web_util_noti_get(const char *resource, char **res);

int web_util_json_init(void);
int web_util_json_fini(void);
int web_util_json_begin(void);
int web_util_json_end(void);
int web_util_json_data_array_begin(void);
int web_util_json_data_array_end(void);
int web_util_json_add_int(const char* key, long long int value);
int web_util_json_add_double(const char* key, double value);
int web_util_json_add_boolean(const char* key, bool value);
int web_util_json_add_string(const char* key, const char *value);
int web_util_json_add_sensor_data(const char* sensorpi_id, web_util_sensor_data_s *sensor_data);

/* The returned string is a newly allocated string which ought to be freed after use*/
char *web_util_get_json_string(void);

#endif /* __POSITION_FINDER_WEBUTIL_H__ */
