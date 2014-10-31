#ifndef MOUSECAPTURE_H
#define MOUSECAPTURE_H

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include <string>
#include <vector>

class MouseCapture
{
public:
	MouseCapture();
	MouseCapture(const std::string wName, cv::Mat& frame);
	void loadImg(std::string imgFile);
	bool drawRect();
	static void onMouse(int event, int x, int y, int flag, void *parma);

	cv::Rect rect;
	std::vector<cv::Rect> vrect;

private:
	cv::Mat img;
	std::string winName;
	bool isMarked;
	bool isButtonUp;

};

#endif // MOUSECAPTURE_H