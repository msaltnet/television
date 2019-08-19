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

#include <stdbool.h>
#include <curl/curl.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "log.h"
#include "webutil.h"
#include "feature.h"

#define URI_PATH_LEN 64
#define REQ_CON_TIMEOUT 5L
#define REQ_TIMEOUT 7L
#define LOCAL_DECISION_OF_FEATURE 0
#define CERT_FILE_PATH "/opt/share/cert-svc/ca-certificate.crt"

typedef struct _wu_json_handle {
	JsonBuilder *builder;
	bool is_begin;
	bool is_end;
} wu_json_handle;

static wu_json_handle Json_h = {NULL, false, false};

static size_t _post_response_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t res_size = 0;

	res_size = size*nmemb;

	if (res_size > 0)
		_I("POST response : %s", ptr);
	/* What should we do here, if response body has negative message? */

	return res_size;
}

static size_t _get_response_write_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t res_size = 0;
	char **received = (char **)data;

	res_size = size*nmemb;

	if (received && res_size > 0)
		*received = strndup((char *)ptr, size*nmemb);
	else
		_E("fail to get response [res size : %d]", res_size);

	return res_size;
}

static int __curl_debug(CURL *handle, curl_infotype type,
	char *data, size_t size, void *userptr)
{
	const char *prefix = NULL;
	char *message = NULL;

	switch (type) {
	case CURLINFO_END:
		return 0;
	case CURLINFO_TEXT:
		_D("== text Info: %s", data);
		return 0;
	case CURLINFO_HEADER_OUT:
		prefix = "=> Send header:";
		break;
	case CURLINFO_DATA_OUT:
		prefix = "=> Send data:";
		break;
	case CURLINFO_SSL_DATA_OUT:
		prefix = "=> Send SSL data:";
		break;
	case CURLINFO_HEADER_IN:
		prefix = "<= Recv header:";
		break;
	case CURLINFO_DATA_IN:
		prefix = "<= Recv data:";
		break;
	case CURLINFO_SSL_DATA_IN:
		prefix = "<= Recv SSL data:";
		break;
	}
	message = g_strndup(data, size);
	_D("%s %s", prefix, message);
	g_free(message);
	return 0;
}

int web_util_noti_init(void)
{
	int ret = 0;
	CURLcode result;
	result = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (result != CURLE_OK) {
		_E("curl_global_init() failed: %s",
			curl_easy_strerror(result));
		ret = -1;
	}
	return ret;
}

void web_util_noti_fini(void)
{
	curl_global_cleanup();
	return;
}

int web_util_noti_post_image_data(web_util_image_data_s *post_data, const char *tag,
	const void *image_data, unsigned int image_size)
{
	int ret = 0;
	CURL *curl = NULL;
	CURLcode response = CURLE_OK;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	char *filename = NULL;
	char *post_url = NULL;

	retv_if(post_data == NULL, -1);
	retv_if(tag == NULL, -1);
	retv_if(image_data == NULL, -1);
	retv_if(image_size == 0, -1);

	curl = curl_easy_init();

	if (!curl) {
		_E("fail to init curl");
		return -1;
	}

	filename = g_strdup_printf("%s.jpg", tag);
	post_url = g_strdup_printf("%s?tag=%s&id=%s", post_data->url, tag, post_data->path);

  _D("FileName: [%s], PostUrl: [%s]", filename, post_url);

	curl_formadd(&formpost, &lastptr,
		CURLFORM_COPYNAME, "content-type:",
		CURLFORM_COPYCONTENTS, "multipart/form-data",
		CURLFORM_END);

	curl_formadd(&formpost, &lastptr,
		CURLFORM_COPYNAME, "imageFile",
		CURLFORM_BUFFER, filename,
		CURLFORM_BUFFERPTR, image_data,
		CURLFORM_BUFFERLENGTH, image_size,
		CURLFORM_END);

	curl_easy_setopt(curl, CURLOPT_URL, post_url);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	/* if CURLOPT_VERBOSE is enabled, __curl_debug() function will be called */
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, __curl_debug);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, REQ_CON_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQ_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CAPATH, CERT_FILE_PATH);

	response = curl_easy_perform(curl);

	if (response != CURLE_OK) {
		_E("curl_easy_perform() failed: %s",
			curl_easy_strerror(response));
		ret = -1;
	}

	curl_easy_cleanup(curl);
	curl_formfree(formpost);
	g_free(post_url);
	g_free(filename);

	return ret;
}

int web_util_noti_post(const char *resource, const char *json_data)
{
	int ret = 0;
	CURL *curl = NULL;
	CURLcode response = CURLE_OK;
	struct curl_slist *headers = NULL;

	retv_if(resource == NULL, -1);
	retv_if(json_data == NULL, -1);

	_I("server : %s", resource);
	_I("json_data : %s", json_data);

	curl = curl_easy_init();

	if (!curl) {
		_E("fail to init curl");
		return -1;
	}

	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, resource);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _post_response_write_callback);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, REQ_CON_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQ_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CAPATH, CERT_FILE_PATH);

	response = curl_easy_perform(curl);

	if (response != CURLE_OK) {
		_E("curl_easy_perform() failed: %s",
			curl_easy_strerror(response));
		/* What should we do here, if response is kind of errors? */
		ret = -1;
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return ret;
}

int web_util_noti_get(const char *resource, char **res)
{
	int ret = 0;
	CURL *curl = NULL;
	CURLcode response = CURLE_OK;

	retv_if(resource == NULL, -1);

	_I("GET to [%s]", resource);

	curl = curl_easy_init();

	if (!curl) {
		_E("fail to init curl");
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, resource);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _get_response_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)res);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "tizen-iot-agent/1.0");
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, REQ_CON_TIMEOUT);

	response = curl_easy_perform(curl);

	if (response != CURLE_OK) {
		_E("curl_easy_perform() failed: %s",
		curl_easy_strerror(response));
		/* What should we do here, if response is kind of errors? */
		ret = -1;
	}

	curl_easy_cleanup(curl);

	return ret;
}

int web_util_json_init(void)
{
	if (Json_h.builder)
		g_object_unref(Json_h.builder);

	Json_h.is_begin = false;
	Json_h.is_end = false;
	Json_h.builder = json_builder_new();
	retv_if(Json_h.builder == NULL, -1);

	return 0;
}

int web_util_json_fini(void)
{

	if (Json_h.builder) {
		g_object_unref(Json_h.builder);
		Json_h.builder = NULL;
	}

	Json_h.is_begin = false;
	Json_h.is_end = false;

	return 0;
}

int web_util_json_begin(void)
{
	retv_if(Json_h.builder == NULL, -1);
	retv_if(Json_h.is_begin == true, -1);
	retv_if(Json_h.is_end == true, -1);

	Json_h.is_begin = true;

	json_builder_begin_object(Json_h.builder);

	return 0;
}

int web_util_json_end(void)
{
	retv_if(Json_h.builder == NULL, -1);
	retv_if(Json_h.is_begin == false, -1);
	retv_if(Json_h.is_end == true, -1);

	json_builder_end_object(Json_h.builder);
	Json_h.is_end = true;

	return 0;
}

int web_util_json_add_int(const char* key, long long int value)
{
	retv_if(!key, -1);

	if (Json_h.builder == NULL) {
		_E("Handle for json is not initialized, call web_util_json_init() first");
		return -1;
	}

	if (Json_h.is_begin == false) {
		_E("json object has not begun, call web_util_json_begin() first");
		return -1;
	}

	if (Json_h.is_end == true) {
		_E("json object has already ended, call web_util_json_begin() first");
		return -1;
	}

	json_builder_set_member_name(Json_h.builder, key);
	json_builder_add_int_value(Json_h.builder, value);

	return 0;
}

int web_util_json_add_double(const char* key, double value)
{
	retv_if(!key, -1);

	if (Json_h.builder == NULL) {
		_E("Handle for json is not initialized, call web_util_json_init() first");
		return -1;
	}

	if (Json_h.is_begin == false) {
		_E("json object has not begun, call web_util_json_begin() first");
		return -1;
	}

	if (Json_h.is_end == true) {
		_E("json object has already ended, call web_util_json_begin() first");
		return -1;
	}

	json_builder_set_member_name(Json_h.builder, key);
	json_builder_add_double_value(Json_h.builder, value);

	return 0;
}

int web_util_json_add_boolean(const char* key, bool value)
{
	retv_if(!key, -1);

	if (Json_h.builder == NULL) {
		_E("Handle for json is not initialized, call web_util_json_init() first");
		return -1;
	}

	if (Json_h.is_begin == false) {
		_E("json object has not begun, call web_util_json_begin() first");
		return -1;
	}

	if (Json_h.is_end == true) {
		_E("json object has already ended, call web_util_json_begin() first");
		return -1;
	}

	json_builder_set_member_name(Json_h.builder, key);
	json_builder_add_boolean_value(Json_h.builder, value);

	return 0;
}

int web_util_json_add_string(const char* key, const char *value)
{
	retv_if(!key, -1);

	if (Json_h.builder == NULL) {
		_E("Handle for json is not initialized, call web_util_json_init() first");
		return -1;
	}

	if (Json_h.is_begin == false) {
		_E("json object has not begun, call web_util_json_begin() first");
		return -1;
	}

	if (Json_h.is_end == true) {
		_E("json object has already ended, call web_util_json_begin() first");
		return -1;
	}

	json_builder_set_member_name(Json_h.builder, key);
	json_builder_add_string_value(Json_h.builder, value);

	return 0;
}

int web_util_json_data_array_begin(void)
{
	int ret = 0;
	retv_if(Json_h.builder == NULL, -1);

	ret = web_util_json_begin();
	retv_if(ret, -1);

	json_builder_set_member_name(Json_h.builder, "SensorDataList");
	json_builder_begin_array(Json_h.builder);

	return 0;
}

int web_util_json_data_array_end(void)
{
	retv_if(Json_h.builder == NULL, -1);
	retv_if(Json_h.is_begin == false, -1);
	retv_if(Json_h.is_end == true, -1);

	json_builder_end_array(Json_h.builder);
	web_util_json_end();

	return 0;
}

int web_util_json_add_sensor_data(const char* sensorpi_id, web_util_sensor_data_s *sensor_data)
{
	const char n_id[] = "sensorId";
	const char n_tag[] = "tag";
	const char n_motion[] = "motion";
	const char n_co2[] = "co2";
	const char n_sound[] = "sound";
	const char n_ip[] = "ip";

	retv_if(!sensorpi_id, -1);
	retv_if(Json_h.builder == NULL, -1);
	retv_if(Json_h.is_begin == false, -1);
	retv_if(Json_h.is_end == true, -1);
	retv_if(sensor_data == NULL, -1);

	/* JSON structure :
	{
		SensorPiID: string,
		SensorPiIP: string,
		Motion: boolean,
		Flame: boolean,
		Humidity: double,
		Temperature: double,
		Vibration: boolean,
		CO2: double,
		SoundLevel: int,
		Tilt: int,
		Light: int,
		Collision: int,
		Obstacle: int,
		Distance: double,
		Rain: int,
		Touch: int,
		Gas: int,
		SensorEnabled: [Motion, ],
		Hash: string,
	}
	*/

	web_util_json_add_string(n_id, sensorpi_id);
	web_util_json_add_string(n_tag, sensor_data->tag);

	if (sensor_data->ip_addr)
		web_util_json_add_string(n_ip, sensor_data->ip_addr);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_MOTION)
		web_util_json_add_int(n_motion, sensor_data->motion);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_CO2)
		web_util_json_add_int(n_co2, sensor_data->co2);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_SOUND)
		web_util_json_add_int(n_sound, sensor_data->soundlevel);

#if LOCAL_DECISION_OF_FEATURE
	const char n_camera[] = "camera";
	const char n_feature[] = "feature";

	json_builder_set_member_name(Json_h.builder, n_feature);
	json_builder_begin_object(Json_h.builder);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_MOTION)
		web_util_json_add_boolean(n_motion, true);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_CO2)
		web_util_json_add_boolean(n_co2, true);

	if (sensor_data->enabled_feature & FEATURE_SENSOR_SOUND)
		web_util_json_add_boolean(n_sound, true);

	if (sensor_data->enabled_feature & FEATURE_CAMERA)
		web_util_json_add_boolean(n_camera, true);

	json_builder_end_object(Json_h.builder);
#endif

	return 0;
}

char *web_util_get_json_string(void)
{
	JsonGenerator *gen = NULL;
	JsonNode *root = NULL;
	char *str = NULL;

	retv_if(Json_h.builder == NULL, NULL);
	retv_if(Json_h.is_begin == false, NULL);
	retv_if(Json_h.is_end == false, NULL);

	root = json_builder_get_root(Json_h.builder);
	retv_if(root == NULL, NULL);

	gen = json_generator_new();
	goto_if(gen == NULL, error_release_all);
	json_generator_set_root(gen, root);

	str = json_generator_to_data(gen, NULL);

	return str;

error_release_all:
	if (root)
		json_node_free(root);

	return NULL;
}

