#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "TYApi.h"

inline void ensure_directory_exists(const std::string &path) {
	if (path.empty()) {
		return;
	}
#ifdef _WIN32
	_mkdir(path.c_str());
#else
	mkdir(path.c_str(), 0755);
#endif
}

inline std::string prepare_debug_directory(const std::string &subdir) {
	const std::string base = "debug_outputs";
	ensure_directory_exists(base);
	if (subdir.empty()) {
		return base;
	}
	const std::string full = base + "/" + subdir;
	ensure_directory_exists(full);
	return full;
}

inline int clamp_to_range(int value, int min_value, int max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

inline bool dump_uint16_matrix_txt(const std::string &file_path, const uint16_t *data, int width, int height, const char *label) {
	if (!data || width <= 0 || height <= 0) {
		return false;
	}

	FILE *fp = fopen(file_path.c_str(), "w");
	if (!fp) {
		return false;
	}

	if (label) {
		fprintf(fp, "# %s\n", label);
	}
	fprintf(fp, "width=%d\nheight=%d\n", width, height);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const uint16_t value = data[y * width + x];
			fprintf(fp, "%u", static_cast<unsigned int>(value));
			if (x < width - 1) {
				fprintf(fp, ",");
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

inline bool dump_rgb_bgr_txt(const std::string &file_path, const uint8_t *data, int width, int height, const char *label) {
	if (!data || width <= 0 || height <= 0) {
		return false;
	}

	FILE *fp = fopen(file_path.c_str(), "w");
	if (!fp) {
		return false;
	}

	if (label) {
		fprintf(fp, "# %s\n", label);
	}
	fprintf(fp, "width=%d\nheight=%d\nchannels=3 (B,G,R order)\n", width, height);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int idx = (y * width + x) * 3;
			const uint8_t b = data[idx + 0];
			const uint8_t g = data[idx + 1];
			const uint8_t r = data[idx + 2];
			fprintf(fp, "%u,%u,%u", static_cast<unsigned int>(b), static_cast<unsigned int>(g), static_cast<unsigned int>(r));
			if (x < width - 1) {
				fprintf(fp, " ");
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

inline bool convert_to_bgr_buffer(const uint8_t *src, int width, int height, TY_PIXEL_FORMAT format, std::vector<uint8_t> &dst) {
	if (!src || width <= 0 || height <= 0) {
		return false;
	}

	dst.assign(width * height * 3, 0);
	switch (format) {
		case TY_PIXEL_FORMAT_BGR: {
			std::memcpy(dst.data(), src, dst.size());
			return true;
		}
		case TY_PIXEL_FORMAT_RGB: {
			for (int i = 0; i < width * height; ++i) {
				dst[i * 3 + 0] = src[i * 3 + 2];
				dst[i * 3 + 1] = src[i * 3 + 1];
				dst[i * 3 + 2] = src[i * 3 + 0];
			}
			return true;
		}
		case TY_PIXEL_FORMAT_YUYV: {
			const int pixelCount = width * height;
			for (int srcIdx = 0, dstIdx = 0; srcIdx < pixelCount * 2; srcIdx += 4, dstIdx += 6) {
				const int y0 = src[srcIdx + 0];
				const int u = src[srcIdx + 1];
				const int y1 = src[srcIdx + 2];
				const int v = src[srcIdx + 3];

				const int b0 = y0 + 1.772 * (u - 128);
				const int g0 = y0 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
				const int r0 = y0 + 1.402 * (v - 128);

				const int b1 = y1 + 1.772 * (u - 128);
				const int g1 = y1 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
				const int r1 = y1 + 1.402 * (v - 128);

				dst[dstIdx + 0] = static_cast<uint8_t>(clamp_to_range(b0, 0, 255));
				dst[dstIdx + 1] = static_cast<uint8_t>(clamp_to_range(g0, 0, 255));
				dst[dstIdx + 2] = static_cast<uint8_t>(clamp_to_range(r0, 0, 255));

				dst[dstIdx + 3] = static_cast<uint8_t>(clamp_to_range(b1, 0, 255));
				dst[dstIdx + 4] = static_cast<uint8_t>(clamp_to_range(g1, 0, 255));
				dst[dstIdx + 5] = static_cast<uint8_t>(clamp_to_range(r1, 0, 255));
			}
			return true;
		}
		default:
			break;
	}
	return false;
}


