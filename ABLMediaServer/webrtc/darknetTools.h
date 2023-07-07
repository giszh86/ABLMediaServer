#pragma once


#include "HSingleton.h"

#include <opencv2/opencv.hpp>



class darknetTools
{
public:
	darknetTools() {};


	~darknetTools() {};
	


	// Convert YUV data to cv::Mat
	cv::Mat convertYUVtoMat(unsigned char* yuvData, int width, int height) {
		cv::Mat yuvImg(height + height / 2, width, CV_8UC1, yuvData);
		cv::Mat bgrImg;
		cv::cvtColor(yuvImg, bgrImg, cv::COLOR_YUV2BGR_I420);
		return bgrImg;
	}

	void convertMatToYUV420(const cv::Mat& bgrImg, unsigned char* yuvData, int& width, int& height) {
		cv::Mat yuvImg;
		cv::cvtColor(bgrImg, yuvImg, cv::COLOR_BGR2YUV_I420);

		// Get the width and height of the image
		width = bgrImg.cols;
		height = bgrImg.rows;

		// Copy Y channel (Grayscale)
		int ySize = width * height;
		std::memcpy(yuvData, yuvImg.data, ySize);

		// Copy U and V channels (Chroma)
		int uSize = width * height / 4;
		std::memcpy(yuvData + ySize, yuvImg.data + ySize, uSize);
		std::memcpy(yuvData + ySize + uSize, yuvImg.data + ySize + uSize, uSize);
	}

	std::list<std::string> readLinesToList(const std::string& filename) {
		std::list<std::string> lines;
		std::ifstream file(filename);

		if (!file.is_open()) {
			std::cerr << "Error opening file: " << filename << std::endl;
			return lines;
		}

		std::string line;
		while (std::getline(file, line)) {
			lines.push_back(line);
		}

		file.close();
		return lines;
	}

	std::string get_line_value(const std::list<std::string>& lines, int line_number) {
		if (line_number < 1 || line_number > lines.size()) {
			std::cerr << "Invalid line number: " << line_number << std::endl;
			return "";
		}

		auto it = lines.begin();
		std::advance(it, line_number - 1);
		return *it;
	}

};



#define gbldarknetToolsGet HSingletonTemplatePtr<darknetTools>::Instance()





