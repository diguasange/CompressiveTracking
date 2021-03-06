﻿#include "CompressiveTracker.h"
#include <math.h>
#include <iostream>
using namespace cv;
using namespace std;

#define PRINT(x) cout<<(#x)<<": "<<(x)<<"  ";

//------------------------------------------------
CompressiveTracker::CompressiveTracker(void)
{
	featureMinNumRect = 2;
	featureMaxNumRect = 4;	// number of rectangle from 2 to 4
	featureNum = 100;// 50;	// number of all weaker classifiers, i.e,feature pool
	rOuterPositive = 4;	// radical scope of positive samples
	rSearchWindow = 25; // size of search window
	muPositive = vector<float>(featureNum, 0.0f);
	muNegative = vector<float>(featureNum, 0.0f);
	sigmaPositive = vector<float>(featureNum, 1.0f);
	sigmaNegative = vector<float>(featureNum, 1.0f);
	learnRate = 0.85f;	// Learning rate parameter

	rFineSearchWindow = 10;
	InitScales(10, 0.8, 1.2);
}



CompressiveTracker::~CompressiveTracker(void)
{
}



void CompressiveTracker::InitScales(int scalenum, float st, float en)
{
	float step = (en - st) / scalenum;
	for (int i = 0; i < scalenum; i++)
	{
		float temp = st + step*i;
		scales.push_back(temp);
	}
}


//Í¨¹ý»ý·ÖÍ¼À´¼ÆËã²É¼¯µ½µÄÃ¿Ò»¸öÑù±¾µÄharrÌØÕ÷£¬Õâ¸öÌØÕ÷Í¨¹ýÓëfeaturesWeightÀ´Ïà³Ë  
//¾ÍÏàµ±ÓÚÍ¶Ó°µ½Ëæ»ú²âÁ¿¾ØÕóÖÐÁË£¬Ò²¾ÍÊÇ½øÐÐÏ¡Êè±í´ïÁË¡£
//Ã¿Ò»¸öÑù±¾ÓÐÄ¬ÈÏÎª50¸öharrÌØÕ÷£¬Ã¿Ò»¸öharrÌØÕ÷ÊÇÓÉ2µ½3¸öËæ»úÑ¡ÔñµÄ¾ØÐÎ¿òÀ´¹¹³ÉµÄ£¬  
//¶ÔÕâÐ©¾ØÐÎ¿òµÄ»Ò¶È¼ÓÈ¨ÇóºÍ×÷ÎªÕâÒ»¸öharrÌØÕ÷µÄÌØÕ÷Öµ¡£ 
void CompressiveTracker::HaarFeature(Rect& _objectBox, int _numFeature)
/*Description: compute Haar features
  Arguments:
  -_objectBox: [x y width height] object rectangle
  -_numFeature: total number of features.The default is 50.
*/
{
	features = vector<vector<Rect_<float>>>(_numFeature, vector<Rect_<float>>());
	featuresWeight = vector<vector<float>>(_numFeature, vector<float>());
	
	int numRect;
	Rect rectTemp;
	float weightTemp;
      
	//for (int i=0; i<_numFeature; i++)
	//{
	//	numRect = cvFloor(rng.uniform((double)featureMinNumRect, (double)featureMaxNumRect));
	//
	//	for (int j=0; j<numRect; j++)
	//	{
	//		rectTemp.x = cvFloor(rng.uniform(2.0, (double)(_objectBox.width - 5)));// origin:0  3
	//		rectTemp.y = cvFloor(rng.uniform(2.0, (double)(_objectBox.height - 5)));
	//		rectTemp.width = cvCeil(rng.uniform(2.0, (double)(_objectBox.width - rectTemp.x - 2)));
	//		rectTemp.height = cvCeil(rng.uniform(2.0, (double)(_objectBox.height - rectTemp.y - 2)));
	//		features[i].push_back(rectTemp);

	//		//weightTemp = (float)pow(-1.0, cvFloor(rng.uniform(0.0, 2.0))) / sqrt(float(numRect));
	//		weightTemp = (float)pow(-1.0, cvFloor(rng.uniform(0.0, 2.0))) / (rectTemp.width*rectTemp.height);//new
	//		featuresWeight[i].push_back(weightTemp);
 //          
	//	}
	//}


	for (int i = 0; i<_numFeature; i++)
	{
		numRect = cvFloor(rng.uniform((double)featureMinNumRect, (double)featureMaxNumRect));

		for (int j = 0; j<numRect; j++)
		{
			rectTemp.x = (float)cvFloor(rng.uniform(2.0, (double)(_objectBox.width - 5)));// origin:0  3
			rectTemp.y = (float)cvFloor(rng.uniform(2.0, (double)(_objectBox.height - 5)));
			rectTemp.width = (float)cvCeil(rng.uniform(2.0, (double)(_objectBox.width - rectTemp.x - 2)));
			rectTemp.height = (float)cvCeil(rng.uniform(2.0, (double)(_objectBox.height - rectTemp.y - 2)));
			features[i].push_back(rectTemp);

			//weightTemp = (float)pow(-1.0, cvFloor(rng.uniform(0.0, 2.0))) / sqrt(float(numRect));
			weightTemp = (float)pow(-1.0, cvFloor(rng.uniform(0.0, 2.0))) / (rectTemp.width*rectTemp.height);//new
			featuresWeight[i].push_back(weightTemp);

		}
	}



}



//ÔÚÉÏÒ»Ö¡¸ú×ÙµÄÄ¿±êboxµÄÖÜÎ§²É¼¯Èô¸ÉÕýÑù±¾ºÍ¸ºÑù±¾£¬À´³õÊ¼»¯»òÕß¸üÐÂ·ÖÀàÆ÷µÄ  
void CompressiveTracker::sampleRect(Mat& _image, Rect& _objectBox, float _rInner, float _rOuter, int _maxSampleNum, vector<Rect>& _sampleBox)
/* Description: compute the coordinate of positive and negative sample image templates
   Arguments:
   -_image:        processing frame
   -_objectBox:    recent object position 
   -_rInner:       inner sampling radius
   -_rOuter:       Outer sampling radius
   -_maxSampleNum: maximal number of sampled images
   -_sampleBox:    Storing the rectangle coordinates of the sampled images.
*/
{
	int rowsz = _image.rows - _objectBox.height - 1;
	int colsz = _image.cols - _objectBox.width - 1;
	float inradsq = _rInner*_rInner;
	float outradsq = _rOuter*_rOuter;

  	
	int dist;

	int minrow = max(0,(int)_objectBox.y-(int)_rInner);
	int maxrow = min((int)rowsz-1,(int)_objectBox.y+(int)_rInner);
	int mincol = max(0,(int)_objectBox.x-(int)_rInner);
	int maxcol = min((int)colsz-1,(int)_objectBox.x+(int)_rInner);
    
	
	
	int i = 0;

	float prob = ((float)(_maxSampleNum))/(maxrow-minrow+1)/(maxcol-mincol+1);

	int r;
	int c;
    
    _sampleBox.clear();//important
    Rect rec(0,0,0,0);

	for( r=minrow; r<=(int)maxrow; r++ )
		for( c=mincol; c<=(int)maxcol; c++ ){
			//¼ÆËãÉú³ÉµÄboxµ½Ä¿±êboxµÄ¾àÀë 
			dist = (_objectBox.y-r)*(_objectBox.y-r) + (_objectBox.x-c)*(_objectBox.x-c);

			//ºóÁ½¸öÌõ¼þÊÇ±£Ö¤¾àÀëÐèÒªÔÚ_rInnerºÍ_rOuterµÄ·¶Î§ÄÚ  
			//ÄÇÃ´rng.uniform(0.,1.) < prob Õâ¸öÊÇ¸ÉÂïµÄÄØ£¿  
			//Á¬×ÅÉÏÃæ¿´£¬Èç¹û_maxSampleNum´óÓÚÄÇ¸ö×î´ó¸öÊý£¬prob¾Í´óÓÚ1£¬ÕâÑù£¬  
			//rng.uniform(0.,1.) < probÕâ¸öÌõ¼þ¾Í×ÜÄÜÂú×ã£¬±íÊ¾ÔÚÕâ¸ö·¶Î§²úÉúµÄ  
			//ËùÒÔboxÎÒ¶¼ÒªÁË£¨ÒòÎªÎÒ±¾ÉíÏëÒª¸ü¶àµÄ£¬µ«ÊÇÄã¸ø²»ÁËÎÒÄÇÃ´¶à£¬ÄÇÃ´ÄãÄÜ¸øµÄ£¬ÎÒ¿Ï¶¨È«ÒªÁË£©¡£  
			//ÄÇÈç¹ûÄã¸øµÄÌ«¶àÁË£¬ÎÒ²»ÒªÄÇÃ´¶à£¬Ò²¾ÍÊÇprob<1£¬ÄÇÎÒ¾ÍËæ»úµØÌø¼¸¸ö×ßºÃÁË  

			// ÎªÊ²Ã´ÊÇ´óÓÚoutÐ¡ÓÚinµÄ·¶Î§ÄÚ²ÉÑù
			if( rng.uniform(0.,1.)<prob && dist < inradsq && dist >= outradsq ){
				
                rec.x = c;
				rec.y = r;
				rec.width = _objectBox.width;
				rec.height= _objectBox.height;
				
                _sampleBox.push_back(rec);				
				
				i++;
			}
		}
	
		_sampleBox.resize(i);
		
}



//Õâ¸ösampleRectµÄÖØÔØº¯ÊýÊÇÓÃÀ´ÔÚÉÏÒ»Ö¡¸ú×ÙµÄÄ¿±êboxµÄÖÜÎ§£¨¾àÀëÐ¡ÓÚ_srw£©²É¼¯Èô¸ÉboxÀ´´ý¼ì²â¡£  
//ÓëÉÏÃæµÄÄÇ¸ö²»Ò»Ñù£¬ÉÏÃæÄÇ¸öÊÇÔÚÕâÒ»Ö¡ÒÑ¾­¼ì²â³öÄ¿±êµÄ»ù´¡ÉÏ£¬²É¼¯Õý¸ºÑù±¾À´¸üÐÂ·ÖÀàÆ÷µÄ¡£  
//ÉÏÃæÄÇ¸öÊôÓÚÂÛÎÄÖÐÌáµ½µÄËã·¨µÄµÚËÄ¸ö²½Öè£¬Õâ¸öÊÇµÚÒ»¸ö²½Öè¡£
void CompressiveTracker::sampleRect(Mat& _image, Rect& _objectBox, float _srw, vector<Rect>& _sampleBox)
/* Description: Compute the coordinate of samples when detecting the object.*/
{
	int rowsz = _image.rows - _objectBox.height - 1;
	int colsz = _image.cols - _objectBox.width - 1;
	float inradsq = _srw*_srw;	
	

	int dist;

	int minrow = max(0,(int)_objectBox.y-(int)_srw);
	int maxrow = min((int)rowsz-1,(int)_objectBox.y+(int)_srw);
	int mincol = max(0,(int)_objectBox.x-(int)_srw);
	int maxcol = min((int)colsz-1,(int)_objectBox.x+(int)_srw);

	int i = 0;

	int r;
	int c;

	Rect rec(0,0,0,0);
    _sampleBox.clear();//important

	for( r=minrow; r<=(int)maxrow; r++ )
		for( c=mincol; c<=(int)maxcol; c++ ){
			dist = (_objectBox.y-r)*(_objectBox.y-r) + (_objectBox.x-c)*(_objectBox.x-c);

			if( dist < inradsq ){

				rec.x = c;
				rec.y = r;
				rec.width = _objectBox.width;
				rec.height= _objectBox.height;

				_sampleBox.push_back(rec);				

				i++;
			}
		}
	
		_sampleBox.resize(i);

}


// Compute the features of samples
void CompressiveTracker::getFeatureValue(Mat& _imageIntegral, vector<Rect>& _sampleBox, Mat& _sampleFeatureValue)
{
	int sampleBoxSize = _sampleBox.size();
	_sampleFeatureValue.create(featureNum, sampleBoxSize, CV_32F);
	float tempValue;
	int xMin;
	int xMax;
	int yMin;
	int yMax;

	for (int i=0; i<featureNum; i++)//Ëæ»úµÄ50¸öhaarÌØÕ÷ Ã¿¸öhaarÌØÕ÷ÓÐfeatures.size()¸öËæ»ú¾ØÐÎ
	{
		for (int j=0; j<sampleBoxSize; j++)//sampleBoxSizeÊÇ²ÉÑùµÄ¸öÊý£¬sampleBoxÊÇobjecBox¸½½ü²ÉÑùµÄBox£¨ÓÐx£¬y¡£width height´óÐ¡ºÍobjecµÄÒ»Ñù£©
		{
			tempValue = 0.0f;
			for (size_t k=0; k<features[i].size(); k++)
			{
				xMin = _sampleBox[j].x + features[i][k].x;
				xMax = _sampleBox[j].x + features[i][k].x + features[i][k].width;
				yMin = _sampleBox[j].y + features[i][k].y;
				yMax = _sampleBox[j].y + features[i][k].y + features[i][k].height;
				tempValue += featuresWeight[i][k] * 
					(_imageIntegral.at<float>(yMin, xMin) +
					_imageIntegral.at<float>(yMax, xMax) -
					_imageIntegral.at<float>(yMin, xMax) -
					_imageIntegral.at<float>(yMax, xMin));
			}
			_sampleFeatureValue.at<float>(i,j) = tempValue;
			//50¸öhaarÌØÕ÷£¬´æÏÂÃ¿¸öÌØÕ÷ÖÜÎ§µÄsampleBoxSize¸ö²ÉÑù¿ò»ý·ÖÍ¼ÏñÖµ
		}
	}
}

// Update the mean and variance of the gaussian classifier
void CompressiveTracker::classifierUpdate(Mat& _sampleFeatureValue, vector<float>& _mu, vector<float>& _sigma, float _learnRate)
{
	Scalar muTemp;
	Scalar sigmaTemp;
    
	for (int i=0; i<featureNum; i++)
	{
		//¼ÆËãËùÓÐÕýÑù±¾»òÕß¸ºÑù±¾µÄÄ³¸öharrÌØÕ÷µÄÆÚÍûºÍ±ê×¼²î 
		meanStdDev(_sampleFeatureValue.row(i), muTemp, sigmaTemp);
	   
		_sigma[i] = (float)sqrt( _learnRate*_sigma[i]*_sigma[i]	+ (1.0f-_learnRate)*sigmaTemp.val[0]*sigmaTemp.val[0] 
		+ _learnRate*(1.0f-_learnRate)*(_mu[i]-muTemp.val[0])*(_mu[i]-muTemp.val[0]));	// equation 6 in paper

		_mu[i] = _mu[i]*_learnRate + (1.0f-_learnRate)*muTemp.val[0];	// equation 6 in paper
	}
}


// Compute the ratio classifier 
void CompressiveTracker::radioClassifier(vector<float>& _muPos, vector<float>& _sigmaPos, vector<float>& _muNeg, vector<float>& _sigmaNeg,
										 Mat& _sampleFeatureValue, float& _radioMax, int& _radioMaxIndex)
{
	float sumRadio;
	_radioMax = -FLT_MAX;
	_radioMaxIndex = 0;
	float pPos;
	float pNeg;
	int sampleBoxNum = _sampleFeatureValue.cols;

	for (int j=0; j<sampleBoxNum; j++)//¶ÔÃ¿Ò»¸öBox¼ÆËãÆä·ÖÊýsumRadio£¬µÃ·Ö×î¸ßµÄ¼´Îª±íÃ÷ÆäÊôÓÚÕýÑù±¾µÄ¸ÅÂÊÔ½´ó£¬¼´ÎªÄ¿±êµÄ¿ÉÄÜÐÔÔ½´ó
	{
		sumRadio = 0.0f;
		for (int i=0; i<featureNum; i++)//featureNum¶ÔÓ¦ÎªÂÛÎÄÖÐ¹«Ê½4µÄn
		{
			pPos = exp( (_sampleFeatureValue.at<float>(i,j)-_muPos[i])*(_sampleFeatureValue.at<float>(i,j)-_muPos[i]) / -(2.0f*_sigmaPos[i]*_sigmaPos[i]+1e-30) ) / (_sigmaPos[i]+1e-30);
			pNeg = exp( (_sampleFeatureValue.at<float>(i,j)-_muNeg[i])*(_sampleFeatureValue.at<float>(i,j)-_muNeg[i]) / -(2.0f*_sigmaNeg[i]*_sigmaNeg[i]+1e-30) ) / (_sigmaNeg[i]+1e-30);
			sumRadio += log(pPos+1e-30) - log(pNeg+1e-30);	// equation 4
		}
		if (_radioMax < sumRadio)
		{
			_radioMax = sumRadio;
			_radioMaxIndex = j;
		}
	}
	
}

void CompressiveTracker::init(Mat& _frame, Rect& _objectBox)
{
	// compute feature template
	HaarFeature(_objectBox, featureNum);

	// compute sample templates
	sampleRect(_frame, _objectBox, rOuterPositive, 0, 1000000, samplePositiveBox);//0-rOuterPositive(4)·¶Î§ÄÚ²ÉÑù×î´óÎª1000000¸öÕýÑù±¾
	sampleRect(_frame, _objectBox, rSearchWindow*1.5, rOuterPositive+4.0, 50, sampleNegativeBox);// Origin: 100 ÔÚrOutPositive(4)+4-25*1.5£¨37.5£©·¶Î§ÄÚÕÒ×î´óÎª100¸ö¸ºÑù±¾

	integral(_frame, imageIntegral, CV_32F);

	getFeatureValue(imageIntegral, samplePositiveBox, samplePositiveFeatureValue);
	getFeatureValue(imageIntegral, sampleNegativeBox, sampleNegativeFeatureValue);
	classifierUpdate(samplePositiveFeatureValue, muPositive, sigmaPositive, learnRate);
	classifierUpdate(sampleNegativeFeatureValue, muNegative, sigmaNegative, learnRate);
}

void CompressiveTracker::processFrame(Mat& _frame, Rect& _objectBox)
{
	// predict
	sampleRect(_frame, _objectBox, rSearchWindow,detectBox);
	integral(_frame, imageIntegral, CV_32F);
	getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
	int radioMaxIndex;
	float radioMax;
	radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, radioMax, radioMaxIndex);
	_objectBox = detectBox[radioMaxIndex];//¾ßÓÐ×î´ó¸ÅÂÊµÄBoxÉèÎªobjectBox£¬¸üÐÂÊý¾Ý

	// update
	sampleRect(_frame, _objectBox, rOuterPositive, 0.0, 1000000, samplePositiveBox);//0-rOuterPositive(4)·¶Î§ÄÚ²ÉÑù×î´óÎª1000000¸öÕýÑù±¾
	sampleRect(_frame, _objectBox, rSearchWindow*1.5, rOuterPositive+4.0, 50, sampleNegativeBox);//ÔÚrOutPositive(4)+4-25*1.5£¨37.5£©·¶Î§ÄÚÕÒ×î´óÎª100¸ö¸ºÑù±¾
	
	getFeatureValue(imageIntegral, samplePositiveBox, samplePositiveFeatureValue);
	getFeatureValue(imageIntegral, sampleNegativeBox, sampleNegativeFeatureValue);
	classifierUpdate(samplePositiveFeatureValue, muPositive, sigmaPositive, learnRate);
	classifierUpdate(sampleNegativeFeatureValue, muNegative, sigmaNegative, learnRate);
}







//--------------------------------------------FCT---------------------------------------------

/*在上一帧跟踪的目标box的周围采集若干正样本和负样本，来初始化或者更新分类器的*/
void CompressiveTracker::sampleRectDet(Mat& _image, Rect& _objectBox, float _rInner,int _step, vector<Rect>& _sampleBox)
/* Description: compute the coordinate of positive and negative sample image templates
Arguments:
-_image:        processing frame
-_objectBox:    recent object position
-_rInner:       inner sampling radius
-_step
-_sampleBox:    Storing the rectangle coordinates of the sampled images.
*/
{
	int rowsz = _image.rows - _objectBox.height - 1;
	int colsz = _image.cols - _objectBox.width - 1;
	float inradsq = _rInner*_rInner;


	int dist;

	int minrow = max(0, (int)_objectBox.y - (int)_rInner);
	int maxrow = min((int)rowsz - 1, (int)_objectBox.y + (int)_rInner);
	int mincol = max(0, (int)_objectBox.x - (int)_rInner);
	int maxcol = min((int)colsz - 1, (int)_objectBox.x + (int)_rInner);



	int i = 0;


	int r;
	int c;

	_sampleBox.clear();//important
	Rect rec(0, 0, 0, 0);

	for (r = minrow; r <= (int)maxrow; r+=_step)
	{

		for (c = mincol; c <= (int)maxcol; c+=_step)
		{
			//计算生成的box到目标box的距离 
			dist = (_objectBox.y - r)*(_objectBox.y - r) + (_objectBox.x - c)*(_objectBox.x - c);

			if (dist < inradsq)
			{

				rec.x = c;
				rec.y = r;
				rec.width = _objectBox.width;
				rec.height = _objectBox.height;

				_sampleBox.push_back(rec);

				i++;
			}
		}
	}

	_sampleBox.resize(i);

}


void CompressiveTracker::init_fct(Mat& _frame, Rect& _objectBox)
{
	// compute feature template
	HaarFeature(_objectBox, featureNum);

	// compute sample templates
	sampleRectDet(_frame, _objectBox, rOuterPositive,1, samplePositiveBox);//0-rOuterPositive(4)·¶Î§ÄÚ²ÉÑù×î´óÎª1000000¸öÕýÑù±¾
	sampleRect(_frame, _objectBox, rSearchWindow*1.5, rOuterPositive + 4.0, 50, sampleNegativeBox);// Origin: 100 ÔÚrOutPositive(4)+4-25*1.5£¨37.5£©·¶Î§ÄÚÕÒ×î´óÎª100¸ö¸ºÑù±¾

	integral(_frame, imageIntegral, CV_32F);

	getFeatureValue(imageIntegral, samplePositiveBox, samplePositiveFeatureValue);
	getFeatureValue(imageIntegral, sampleNegativeBox, sampleNegativeFeatureValue);
	classifierUpdate(samplePositiveFeatureValue, muPositive, sigmaPositive, learnRate);
	classifierUpdate(sampleNegativeFeatureValue, muNegative, sigmaNegative, learnRate);
}


void CompressiveTracker::processFrame_fct(Mat& _frame, Rect& _objectBox)
{
	integral(_frame, imageIntegral, CV_32F);
	int radioMaxIndex;
	float radioMax;

	// predict
	sampleRectDet(_frame, _objectBox, rSearchWindow, 4,detectBox);
	getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
	radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, radioMax, radioMaxIndex);
	_objectBox = detectBox[radioMaxIndex];

	sampleRectDet(_frame, _objectBox, rFineSearchWindow, 1, detectBox);
	getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
	radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, radioMax, radioMaxIndex);
	_objectBox = detectBox[radioMaxIndex];


	// update
	sampleRectDet(_frame, _objectBox, rOuterPositive, 1, samplePositiveBox);
	sampleRect(_frame, _objectBox, rSearchWindow*1.5, rOuterPositive + 4.0, 50, sampleNegativeBox);//ÔÚrOutPositive(4)+4-25*1.5£¨37.5£©·¶Î§ÄÚÕÒ×î´óÎª100¸ö¸ºÑù±¾


	getFeatureValue(imageIntegral, samplePositiveBox, samplePositiveFeatureValue);
	getFeatureValue(imageIntegral, sampleNegativeBox, sampleNegativeFeatureValue);
	classifierUpdate(samplePositiveFeatureValue, muPositive, sigmaPositive, learnRate);
	classifierUpdate(sampleNegativeFeatureValue, muNegative, sigmaNegative, learnRate);
}




void CompressiveTracker::processFrame_sfct(Mat& _frame, Rect& _objectBox)
{
	integral(_frame, imageIntegral, CV_32F);
	int radioMaxIndex;
	float radioMax;

	// predict
	sampleRectDet(_frame, _objectBox, rSearchWindow, 4, detectBox);
	getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
	radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, radioMax, radioMaxIndex);
	_objectBox = detectBox[radioMaxIndex];


	vector<int> vradioMaxIndex;
	vector<float> vradioMax;
	vector<Rect> vobjectBox;
	vradioMaxIndex.resize(scales.size());
	vradioMax.resize(scales.size());
	vobjectBox.resize(scales.size());

	for (int i = 0; i < scales.size(); i++)
	{
		Rect object_temp = _objectBox;
		float s1 = 0.5*(scales.at(i) - 1)*object_temp.width;
		float s2 = 0.5*(scales.at(i) - 1)*object_temp.height;
		object_temp.x = round(object_temp.x - s1);
		object_temp.y = round(object_temp.y - s2);
		object_temp.width = round(object_temp.width*scales.at(i));
		object_temp.height = round(object_temp.height*scales.at(i));

		setFeatures(scales.at(i));

		sampleRectDet(_frame, object_temp, rFineSearchWindow, 1, detectBox);
		getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
		radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, vradioMax.at(i), vradioMaxIndex.at(i));
		vobjectBox.at(i) = detectBox[vradioMaxIndex.at(i)];

		resetFeatures(scales.at(i));

		cout << scales.at(i) << ": " << vradioMax.at(i) << endl;
	}



	float maxRadio = FLT_MIN;
	int index = -1;
	for (int i = 0; i < scales.size(); i++)
	{
		if (vradioMax.at(i)>maxRadio)
		{
			maxRadio = vradioMax.at(i);
			index = i;
		}
	}


	//printFeatures();
	//for (int i = 0; i < scales.size(); i++)
	//{
	//	setFeatures(scales.at(i));
	//}

	//for (int i = 0; i < scales.size(); i++)
	//{
	//	resetFeatures(scales.at(i));
	//}
	//
	//printFeatures();






	sampleRectDet(_frame, _objectBox, rFineSearchWindow, 1, detectBox);
	getFeatureValue(imageIntegral, detectBox, detectFeatureValue);
	radioClassifier(muPositive, sigmaPositive, muNegative, sigmaNegative, detectFeatureValue, radioMax, radioMaxIndex);
	_objectBox = detectBox[radioMaxIndex];
	cout << "1.0: " << radioMax << endl;


	if (index != -1 && maxRadio-radioMax>1e-1)
	{
		_objectBox = vobjectBox.at(index);
		setFeatures(scales.at(index));
		cout <<"------------------"<< scales.at(index) <<"-----------------"<< endl;
	}


	// update
	sampleRectDet(_frame, _objectBox, rOuterPositive, 1, samplePositiveBox);
	sampleRect(_frame, _objectBox, rSearchWindow*1.5, rOuterPositive + 4.0, 50, sampleNegativeBox);//ÔÚrOutPositive(4)+4-25*1.5£¨37.5£©·¶Î§ÄÚÕÒ×î´óÎª100¸ö¸ºÑù±¾


	getFeatureValue(imageIntegral, samplePositiveBox, samplePositiveFeatureValue);
	getFeatureValue(imageIntegral, sampleNegativeBox, sampleNegativeFeatureValue);
	classifierUpdate(samplePositiveFeatureValue, muPositive, sigmaPositive, learnRate);
	classifierUpdate(sampleNegativeFeatureValue, muNegative, sigmaNegative, learnRate);
}



void CompressiveTracker::setFeatures(float s)
{
	for (int i = 0; i < featureNum; i++)
	{
		for (int k = 0; k < features.at(i).size(); k++)
		{
			Rect_<float>& rec = features.at(i).at(k);

			rec.x = (rec.x*s);
			rec.y = (rec.y*s);

			rec.width = (rec.width*s);
			rec.height = (rec.height*s);

			featuresWeight.at(i).at(k) /= (s*s);
		}
	}
}


void CompressiveTracker::resetFeatures(float s)
{
	for (int i = 0; i < featureNum; i++)
	{
		for (int k = 0; k < features.at(i).size(); k++)
		{
			Rect_<float>& rec = features.at(i).at(k);

			rec.x = (rec.x/s);
			rec.y = (rec.y/s);

			rec.width = (rec.width/s);
			rec.height = (rec.height/s);

			featuresWeight.at(i).at(k) *= (s*s);
		}
	}
}


void CompressiveTracker::printFeatures()
{
	for (int i = 0; i < featureNum; i++)
	{
		//for (int k = 0; k < features.at(i).size(); k++)
		int k = 0;
		{
			cout<<features.at(i).at(k) << "   ";
			//cout << featuresWeight.at(i).at(k) << "  ";
		}
	}
	cout << endl;
	cout << "--------------------------------------------------------------" << endl;
}