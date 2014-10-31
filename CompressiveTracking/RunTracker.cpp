/************************************************************************
* File:	RunTracker.cpp
* Brief: C++ demo for paper: Kaihua Zhang, Lei Zhang, Ming-Hsuan Yang,"Real-Time Compressive Tracking," ECCV 2012.
* Version: 1.0
* Author: Yang Xian
* Email: yang_xian521@163.com
* Date:	2012/08/03
* History:
* Revised by Kaihua Zhang on 14/8/2012, 23/8/2012
* Email: zhkhua@gmail.com
* Homepage: http://www4.comp.polyu.edu.hk/~cskhzhang/
* Project Website: http://www4.comp.polyu.edu.hk/~cslzhang/CT/CT.htm
************************************************************************/
#include "Config.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <windows.h>
#include "CompressiveTracker.h"
#include "MouseCapture.h"

using namespace cv;
using namespace std;



const int nDataUsed = 0; //使用的数据集，0-original 1-Dataset from CAVIAR

bool readLocation(string locationPath, vector<vector<Rect> > &vR);
void drawLocation(const int fIdx, Mat& img, const vector<vector<Rect> > &vR);
int frameIndex = 0;



int main(int argc, char* argv[])
{
	string configPath = "config.txt";
	Config conf(configPath);

	ofstream outFile;
	if (conf.resultsPath != "")
	{
		outFile.open(conf.resultsPath.c_str(), ios::out);
		if (!outFile)
		{
			cout << "error: could not open results file: " << conf.resultsPath << endl;
			return EXIT_FAILURE;
		}
	}

	//bool useCamera = (conf.sequenceName == "");
	bool useCamera = true;

	VideoCapture cap;

	int startFrame = -1;
	int endFrame = -1;
	string imgFormat;


	if (useCamera)
	{
		//TODO
		cout << "Use The Camere" << endl;

		vector<vector<Rect>> vLocations;

		//cap.open(0);// open the camera
		cap.open("D:\\MyData\\sequences\\ProcessedVideo\\kobe.avi"); //use the video
		if (!readLocation("D:\\MyData\\sequences\\ProcessedVideo\\location.txt", vLocations))
		{
			return EXIT_FAILURE;
		}

		

		if (!cap.isOpened())
		{
			cout << "error:could not open the camere" << endl;
		}


		Mat frame, grayImg, result;

		//CompressiveTracker ct;
		vector<CompressiveTracker> cts;

		vector<Rect> vbox;
		bool havedrawn = false;
		bool stop = false;
		while (1)
		{

			if (stop)
			{
				cts.clear();
				vbox.clear();

				if (frame.empty())
				{
					cout << "error:failed to read the camera buffer " << endl;
					return EXIT_FAILURE;
				}
				cvtColor(frame, grayImg, CV_RGB2GRAY);

				MouseCapture MC("CT", frame);
				havedrawn=MC.drawRect();

				for (int i = 0; i < MC.vrect.size(); i++)
				{

					cts.push_back(CompressiveTracker());
				}

				cout <<"we have created "<< cts.size()<<" treckers" << endl;

				for (int i = 0; i < MC.vrect.size(); i++)
				{
					cts.at(i).init(grayImg, MC.vrect.at(i));
					vbox.push_back(Rect(MC.vrect.at(i)));
				}

				stop = false;
			}

			cap >> frame;
			if (frame.empty())
			{
				cout << "error:failed to read the camera buffer " << endl;
				return EXIT_FAILURE;
			}


			if (havedrawn)
			{
				if (frame.empty())
				{
					cout << "error:failed to read the camera buffer " << endl;
					return EXIT_FAILURE;
				}

				cvtColor(frame, grayImg, CV_RGB2GRAY);

				for (int i = 0; i < cts.size(); i++)
				{
					cts.at(i).processFrame(grayImg, vbox.at(i));
					rectangle(frame, vbox.at(i), Scalar(200, 0, 0), 2);// Draw rectangle
					outFile << i<<": "<<(int)vbox.at(i).x << " " << (int)vbox.at(i).y << " " << (int)vbox.at(i).width << " " << (int)vbox.at(i).height << endl;
				}

				
			}

			drawLocation(frameIndex, frame, vLocations);
			imshow("CT", frame);// Display

			if (waitKey(1*int(!havedrawn)) == 's')
			{
				stop = true;
			}


			frameIndex++;
		}

	}
# pragma region elseif
	else if (0 == nDataUsed)
	{
		// Dataset girl and son on

		string framesFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + conf.sequenceName + "_frames.txt";
		ifstream framesFile(framesFilePath.c_str(), ios::in);
		if (!framesFile)
		{
			cout << "error: could not open sequence frames file: " << framesFilePath << endl;
			return EXIT_FAILURE;
		}
		string framesLine;
		getline(framesFile, framesLine);
		sscanf(framesLine.c_str(), "%d,%d", &startFrame, &endFrame);
		if (framesFile.fail() || startFrame == -1 || endFrame == -1)
		{
			cout << "error: could not parse sequence frames file" << endl;
			return EXIT_FAILURE;
		}
		imgFormat = conf.sequenceBasePath + "/" + conf.sequenceName + "/imgs/img%05d.png";



		// read init box from ground truth file
		string gtFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + conf.sequenceName + "_gt.txt";
		ifstream gtFile(gtFilePath.c_str(), ios::in);
		if (!gtFile)
		{
			cout << "error: could not open sequence gt file: " << gtFilePath << endl;
			return EXIT_FAILURE;
		}
		string gtLine;
		getline(gtFile, gtLine);
		float xmin = -1.f;
		float ymin = -1.f;
		float width = -1.f;
		float height = -1.f;
		sscanf(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);
		if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
		{
			cout << "error: could not parse sequence gt file" << endl;
			return EXIT_FAILURE;
		}




		CompressiveTracker ct;
		Mat frame, grayImg, result;

		Rect box(xmin, ymin, width, height);

		// read first frame to get size
		char imgPath[256];
		sprintf(imgPath, imgFormat.c_str(), startFrame);
		frame = cv::imread(imgPath, 1);
		cvtColor(frame, grayImg, CV_RGB2GRAY);
		ct.init(grayImg, box);

		outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;

		char strFrame[20];
		for (int i = startFrame + 1; i < endFrame; i++)
		{

			char imgPaths[256];
			sprintf(imgPaths, imgFormat.c_str(), i);
			frame = cv::imread(imgPaths, 1);
			if (frame.empty())
			{
				cout << "error: could not read frame: " << imgPath << endl;
				return EXIT_FAILURE;
			}

			cvtColor(frame, grayImg, CV_RGB2GRAY);

			ct.processFrame(grayImg, box);// Process frame

			rectangle(frame, box, Scalar(200, 0, 0), 2);// Draw rectangle

			outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;


			sprintf(strFrame, "#%d ", i);
			putText(frame, strFrame, cvPoint(0, 20), 2, 1, CV_RGB(25, 200, 25));

			imshow("CT", frame);// Display
			waitKey(1);
		}


	}
	else if (1 == nDataUsed)
	{
		string framesFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + conf.sequenceName + "_frames.txt";
		ifstream framesFile(framesFilePath.c_str(), ios::in);
		if (!framesFile)
		{
			cout << "error: could not open sequence frames file: " << framesFilePath << endl;
			return EXIT_FAILURE;
		}
		string framesLine;
		getline(framesFile, framesLine);
		sscanf(framesLine.c_str(), "%d,%d", &startFrame, &endFrame);
		if (framesFile.fail() || startFrame == -1 || endFrame == -1)
		{
			cout << "error: could not parse sequence frames file" << endl;
			return EXIT_FAILURE;
		}

		imgFormat = conf.sequenceBasePath + "/" + conf.sequenceName + "/JPEGS/" + conf.sequenceName + "%04d.jpg";

		//imgFormat = conf.sequenceBasePath + "/" + conf.sequenceName + "/frame_%4d.jpg"; //S2_L1

		// read init box from ground truth file
		string gtFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + conf.sequenceName + "_gt.txt";
		ifstream gtFile(gtFilePath.c_str(), ios::in);
		if (!gtFile)
		{
			cout << "error: could not open sequence gt file: " << gtFilePath << endl;
			return EXIT_FAILURE;
		}
		string gtLine;
		getline(gtFile, gtLine);
		float xmin = -1.f;
		float ymin = -1.f;
		float width = -1.f;
		float height = -1.f;
		sscanf(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);
		if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
		{
			cout << "error: could not parse sequence gt file" << endl;
			return EXIT_FAILURE;
		}



		CompressiveTracker ct;
		Mat frame, grayImg, result;

		Rect box(xmin, ymin, width, height);

		// read first frame to get size
		char imgPath[256];
		sprintf(imgPath, imgFormat.c_str(), startFrame);
		frame = cv::imread(imgPath, 1);
		cvtColor(frame, grayImg, CV_RGB2GRAY);
		ct.init(grayImg, box);

		outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;

		char strFrame[20];
		for (int i = startFrame + 1; i < endFrame; i++)
		{

			char imgPaths[256];
			sprintf(imgPaths, imgFormat.c_str(), i);
			frame = cv::imread(imgPaths, 1);
			if (frame.empty())
			{
				cout << "error: could not read frame: " << imgPath << endl;
				return EXIT_FAILURE;
			}

			cvtColor(frame, grayImg, CV_RGB2GRAY);

			ct.processFrame(grayImg, box);// Process frame

			rectangle(frame, box, Scalar(200, 0, 0), 2);// Draw rectangle

			outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;


			sprintf(strFrame, "#%d ", i);
			putText(frame, strFrame, cvPoint(0, 20), 2, 1, CV_RGB(25, 200, 25));

			imshow("CT", frame);// Display
			waitKey(1);
		}
	} //else if 1
	else if (2 == nDataUsed)
	{
		string framesFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "_frames.txt";
		ifstream framesFile(framesFilePath.c_str(), ios::in);
		if (!framesFile)
		{
			cout << "error: could not open sequence frames file: " << framesFilePath << endl;
			return EXIT_FAILURE;
		}
		string framesLine;
		getline(framesFile, framesLine);
		sscanf(framesLine.c_str(), "%d,%d", &startFrame, &endFrame);
		if (framesFile.fail() || startFrame == -1 || endFrame == -1)
		{
			cout << "error: could not parse sequence frames file" << endl;
			return EXIT_FAILURE;
		}

		imgFormat = conf.sequenceBasePath + "/" + conf.sequenceName + "/frame_%04d.jpg"; //S2_L1

		// read init box from ground truth file
		string gtFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "_gt.txt";
		ifstream gtFile(gtFilePath.c_str(), ios::in);
		if (!gtFile)
		{
			cout << "error: could not open sequence gt file: " << gtFilePath << endl;
			return EXIT_FAILURE;
		}
		string gtLine;
		getline(gtFile, gtLine);
		float xmin = -1.f;
		float ymin = -1.f;
		float width = -1.f;
		float height = -1.f;
		sscanf(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);
		if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
		{
			cout << "error: could not parse sequence gt file" << endl;
			return EXIT_FAILURE;
		}



		CompressiveTracker ct;
		Mat frame, grayImg, result;

		Rect box(xmin, ymin, width, height);

		// read first frame to get size
		char imgPath[256];
		sprintf(imgPath, imgFormat.c_str(), startFrame);
		frame = cv::imread(imgPath, 1);
		cvtColor(frame, grayImg, CV_RGB2GRAY);
		ct.init(grayImg, box);

		outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;

		char strFrame[20];
		for (int i = startFrame + 1; i < endFrame; i++)
		{

			char imgPaths[256];
			sprintf(imgPaths, imgFormat.c_str(), i);
			frame = cv::imread(imgPaths, 1);
			if (frame.empty())
			{
				cout << "error: could not read frame: " << imgPath << endl;
				return EXIT_FAILURE;
			}

			cvtColor(frame, grayImg, CV_RGB2GRAY);

			ct.processFrame(grayImg, box);// Process frame

			rectangle(frame, box, Scalar(200, 0, 0), 2);// Draw rectangle

			outFile << (int)box.x << " " << (int)box.y << " " << (int)box.width << " " << (int)box.height << endl;


			sprintf(strFrame, "#%d ", i);
			putText(frame, strFrame, cvPoint(0, 20), 2, 1, CV_RGB(25, 200, 25));

			imshow("CT", frame);// Display
			waitKey(1);
		}
	} //else if 2
#pragma endregion elseif


	if (outFile.is_open())
	{
		outFile.close();
	}

	return 0;
}



bool readLocation(string locationPath,vector<vector<Rect> > &vR)
{
	ifstream locationInFile(locationPath.c_str(), ios::in);
	
	if (!locationInFile)
	{
		cout << "error: could not open location file: " << locationPath << endl;
		return false;
	}


	string line, name, rect;
	vector<Rect> temp;
	while (getline(locationInFile, line))
	{
		istringstream iss(line);
		rect = "";
		iss >> name >> rect;
		//cout << name << " "<<rect << endl;

		if (rect == "")
		{
			vR.push_back(temp);
			temp.clear();
		}
		else
		{
			float xmin = -1.f;
			float ymin = -1.f;
			float xmax = -1.f;
			float ymax = -1.f;

			sscanf(rect.c_str(),"%f,%f,%f,%f", &xmin, &ymin, &xmax, &ymax);
			temp.push_back(Rect(xmin,ymin,xmax-xmin,ymax-ymin));
		}


	}

	
	vR.erase(vR.begin());//the first of element is invalid


}



void drawLocation(const int fIdx, Mat& img, const vector<vector<Rect> > &vR)
{
	for (int i = 0; i < vR.at(fIdx).size(); i++)
	{
		rectangle(img, vR.at(fIdx).at(i), Scalar(0, 255, 0), 2);
	}

	char strFrame[20];
	sprintf(strFrame, "#%d ", fIdx);
	putText(img, strFrame, cvPoint(0, 20), 2, 1, CV_RGB(25, 200, 25));
}


//void readConfig(char* configFileName, char* imgFilePath, Rect &box);
///*  Description: read the tracking information from file "config.txt"
//Arguments:
//-configFileName: config file name
//-ImgFilePath:    Path of the storing image sequences
//-box:            [x y width height] intial tracking position
//History: Created by Kaihua Zhang on 15/8/2012
//*/
//void readImageSequenceFiles(char* ImgFilePath, vector <string> &imgNames);
///*  Description: search the image names in the image sequences
//Arguments:
//-ImgFilePath: path of the image sequence
//-imgNames:  vector that stores image name
//History: Created by Kaihua Zhang on 15/8/2012
//*/



//int main(int argc, char * argv[])
//{
//
//	char imgFilePath[100];
//    char  conf[100];
//	strcpy(conf,"./config.txt");
//
//	char tmpDirPath[MAX_PATH+1];
//	
//	Rect box; // [x y width height] tracking position
//
//	vector <string> imgNames;
//    
//	readConfig(conf,imgFilePath,box);
//	readImageSequenceFiles(imgFilePath,imgNames);
//
//	// CT framework
//	CompressiveTracker ct;
//
//	Mat frame;
//	Mat grayImg;
//
//	sprintf(tmpDirPath, "%s/", imgFilePath);
//	imgNames[436].insert(0,tmpDirPath);
//	frame = imread(imgNames[436]);
//    cvtColor(frame, grayImg, CV_RGB2GRAY);    
//	ct.init(grayImg, box);    
//
//	char strFrame[20];
//
//    FILE* resultStream;
//	resultStream = fopen("TrackingResults.txt", "w");
//	fprintf (resultStream,"%i %i %i %i\n",(int)box.x,(int)box.y,(int)box.width,(int)box.height);
//
//	for(int i = 437; i < imgNames.size()-1; i ++)
//	{
//		
//		sprintf(tmpDirPath, "%s/", imgFilePath);
//        imgNames[i].insert(0,tmpDirPath);
//        		
//		frame = imread(imgNames[i]);// get frame
//		cvtColor(frame, grayImg, CV_RGB2GRAY);
//		
//		ct.processFrame(grayImg, box);// Process frame
//		
//		rectangle(frame, box, Scalar(200,0,0),2);// Draw rectangle
//
//		fprintf (resultStream,"%i %i %i %i\n",(int)box.x,(int)box.y,(int)box.width,(int)box.height);
//
//		sprintf(strFrame, "#%d ",i) ;
//
//		putText(frame,strFrame,cvPoint(0,20),2,1,CV_RGB(25,200,25));
//		
//		imshow("CT", frame);// Display
//		waitKey(0);		
//	}
//	fclose(resultStream);
//
//	return 0;
//}

//void readConfig(char* configFileName, char* imgFilePath, Rect &box)	
//{
//	int x;
//	int y;
//	int w;
//	int h;
//
//	fstream f;
//	char cstring[1000];
//	int readS=0;
//
//	f.open(configFileName, fstream::in);
//
//	char param1[200]; strcpy(param1,"");
//	char param2[200]; strcpy(param2,"");
//	char param3[200]; strcpy(param3,"");
//
//	f.getline(cstring, sizeof(cstring));
//	readS=sscanf (cstring, "%s %s %s", param1,param2, param3);
//
//	strcpy(imgFilePath,param3);
//
//	f.getline(cstring, sizeof(cstring)); 
//	f.getline(cstring, sizeof(cstring)); 
//	f.getline(cstring, sizeof(cstring));
//
//
//	readS=sscanf (cstring, "%s %s %i %i %i %i", param1,param2, &x, &y, &w, &h);
//
//	box = Rect(x, y, w, h);
//	
//}
//
//
//void readImageSequenceFiles(char* imgFilePath,vector <string> &imgNames)
//{	
//	imgNames.clear();
//
//	char tmpDirSpec[MAX_PATH+1];
//	sprintf (tmpDirSpec, "%s/*", imgFilePath);
//
//	WIN32_FIND_DATAA f;
//	HANDLE h = FindFirstFileA(tmpDirSpec , &f);
//	if(h != INVALID_HANDLE_VALUE)
//	{
//		FindNextFileA(h, &f);	//read ..
//		FindNextFileA(h, &f);	//read .
//		do
//		{
//			imgNames.push_back(f.cFileName);
//		} while(FindNextFileA(h, &f));
//
//	}
//	FindClose(h);	
//}