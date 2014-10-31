#include "MouseCapture.h"
#include <iostream>

using namespace cv;
using namespace std;

void MouseCapture::onMouse(int event, int x, int y, int flag, void *param)
{
	MouseCapture *MC = (MouseCapture *)param;
	switch (event)
	{
	case EVENT_LBUTTONDOWN:
		MC->rect.x = x;
		MC->rect.y = y;
		MC->isButtonUp = false;
		break;

	case EVENT_LBUTTONUP:
		MC->rect.width = x - MC->rect.x;
		MC->rect.height = y - MC->rect.y;
		MC->isButtonUp = true;
		break;

	case EVENT_RBUTTONDOWN:
		MC->isMarked = true;
		break;

	case EVENT_MOUSEMOVE:
		if (flag & EVENT_FLAG_LBUTTON)
		{
			MC->rect.width = x - MC->rect.x;
			MC->rect.height = y - MC->rect.y;
		}
		break;
	default:
		break;
	}

}

MouseCapture::MouseCapture() :winName("MouseControl"), rect(Rect(0, 0, 0, 0)), isMarked(false), isButtonUp(false)
{}

MouseCapture::MouseCapture(const std::string wName, cv::Mat& frame) : winName(wName), img(frame), rect(Rect(0, 0, 0, 0)), isMarked(false), isButtonUp(false)
{}


void MouseCapture::loadImg(string imgFile)
{
	img = imread(imgFile);
}

bool MouseCapture::drawRect()
{
	namedWindow(winName);
	setMouseCallback(winName, onMouse, this);
	Mat tmpImg;
	vrect.clear();
	while (!isMarked)
	{
		tmpImg = img.clone();
		rectangle(tmpImg, rect, Scalar(0, 0, 255), 2);

		if (isButtonUp)
		{
			isButtonUp = false;
			vrect.push_back(rect);
		}

		for (int i = 0; i < vrect.size(); i++)
		{
			rectangle(tmpImg, vrect.at(i), Scalar(0, 0, 255), 2);
		}

		//string str;
		//stringstream strStream;
		//strStream << "Marked Region:" << rect.x << ":" << rect.y << ":" << rect.width << ":" << rect.height;
		//str = strStream.str();
		//putText(tmpImg, str, Point(20, 20), CV_FONT_HERSHEY_COMPLEX, .8, Scalar(0, 255, 0), 1);

		imshow(winName, tmpImg);
		if (waitKey(10) == 'q')
			break;
	}

	cout << "Mark "<<vrect.size()<<" objects finished!" << endl;
	
	return true;
}