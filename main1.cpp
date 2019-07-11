#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/videoio.hpp>

#include<iostream>
#include<conio.h>           

#include "Blob.h"

//voids
void matchCurrentFrameBlobsToExistingBlobs(vector<Blob>& existingBlobs, vector<Blob>& currentFrameBlobs);
void addBlobToExistingBlobs(Blob& currentFrameBlob, vector<Blob>& existingBlobs, int& intIndex);
void addNewBlob(Blob& currentFrameBlob, vector<Blob>& existingBlobs);
double distanceBetweenPoints(Point point1, Point point2);
void drawAndShowContours(Size imageSize, vector< vector< Point> > contours, string strImageName);
void drawAndShowContours(Size imageSize, vector<Blob> blobs, string strImageName);
bool checkIfBlobsCrossedTheLineOut(vector<Blob>& blobs, int& LinePos, int& triggerOut);
void drawBlobInfoOnImage(vector<Blob>& blobs, Mat& imgFrame2Copy);
void drawtriggerOnImage( int& triggerOut, Mat& imgFrame2Copy);


//colours
const Scalar BLACK = Scalar(0.0, 0.0, 0.0);
const Scalar WHITE = Scalar(255.0, 255.0, 255.0);
const Scalar YELLOW = Scalar(0.0, 255.0, 255.0);
const Scalar GREEN = Scalar(0.0, 200.0, 0.0);
const Scalar RED = Scalar(0.0, 0.0, 255.0);



int main(void) {
	VideoCapture capVideo;
	capVideo.open(0);
	Mat imgFrame1;
	Mat imgFrame2;

	//capturing 2 frames
	capVideo.read(imgFrame1);
	capVideo.read(imgFrame2);

	vector<Blob> blobs;			//create the blobs vector
	Point crossingLine[2];		// first point of the array for img1, second point of the array for img2  ..... both points x and y
	int triggerOut = 0;			//if the line is corssed from the top than triggerOut goes up

	//error messages
	if (!capVideo.isOpened()) {
		cout << "\ncannot find video file" << endl << endl;
		return(0);
	}
	


	//create the virtual line for crossing
	int LinePos = (int)round((double)imgFrame1.rows * 0.6);  // in this case, horizontal line at pos rows * x 

	//initiate the crossing line positions
	crossingLine[0].x = 0;
	crossingLine[0].y = LinePos;

	crossingLine[1].x = imgFrame1.cols - 1;
	crossingLine[1].y = LinePos;

	//esc key check
	char CheckForEscKey = 0;

	bool firstFrame = true;
	int frameCount = 2;



	//while loop
	while (capVideo.isOpened() && CheckForEscKey != 27) {

		vector<Blob> currentFrameBlobs;

		Mat imgFrame1Copy = imgFrame1.clone();
		Mat imgFrame2Copy = imgFrame2.clone();


		//constructing the black and white images with only the moving countours
		cvtColor(imgFrame1Copy, imgFrame1Copy, COLOR_BGR2GRAY);
		cvtColor(imgFrame2Copy, imgFrame2Copy, COLOR_BGR2GRAY);

		GaussianBlur(imgFrame1Copy, imgFrame1Copy, Size(5, 5), 0);
		GaussianBlur(imgFrame2Copy, imgFrame2Copy, Size(5, 5), 0);

		Mat imgDifference;
		Mat imgThresh;

		absdiff(imgFrame1Copy, imgFrame2Copy, imgDifference);
		threshold(imgDifference, imgThresh, 30, 255.0, THRESH_BINARY);
		//imshow("imgThresh", imgThresh);

		//start to fill in the countours by dilating every pixel inside the binary by 9, 7, 5, or 3 pixels to fill in a countour
		Mat structuringElement3 = getStructuringElement(MORPH_RECT, Size(3, 3));
		Mat structuringElement5 = getStructuringElement(MORPH_RECT, Size(5, 5));
		Mat structuringElement7 = getStructuringElement(MORPH_RECT, Size(7, 7));
		Mat structuringElement9 = getStructuringElement(MORPH_RECT, Size(9, 9));

		for (unsigned int i = 0; i < 5; i++) {
			dilate(imgThresh, imgThresh, structuringElement3);
			dilate(imgThresh, imgThresh, structuringElement3);
			erode(imgThresh, imgThresh, structuringElement3);
		}


		//find the countours of the moving parts and show it
		vector<vector<Point> > contours;
		Mat imgThreshCopy = imgThresh.clone();

		findContours(imgThreshCopy, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		//drawAndShowContours(imgThresh.size(), contours, "Img Threshold");

		//create the convex hulls and show it
		vector<vector<Point> > convexHulls(contours.size());

		for (unsigned int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], convexHulls[i]);
		}

		//drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");

		// filtering unlikely blobs,
		// every blob is pushed back into the blob vectors
		for (auto& convexHull : convexHulls) {
			Blob possibleBlob(convexHull);

			if (possibleBlob.currentBoundingRect.area() > 400 &&
				possibleBlob.dblCurrentAspectRatio > 0.2 &&
				possibleBlob.dblCurrentAspectRatio < 4.0 &&
				possibleBlob.currentBoundingRect.width > 30 &&
				possibleBlob.currentBoundingRect.height > 30 &&
				possibleBlob.dblCurrentDiagonalSize > 60.0 &&
				(contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
				currentFrameBlobs.push_back(possibleBlob);
			}
		}

		//drawAndShowContours(imgThresh.size(), currentFrameBlobs, "imgCurrentFrameBlobs");


		//update the blob vectors 
		if (firstFrame == true) {
			for (auto& currentFrameBlob : currentFrameBlobs) {
				blobs.push_back(currentFrameBlob);
			}
		}
		else {
			matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
		}

		//drawAndShowContours(imgThresh.size(), blobs, "imgBlobs");

		imgFrame2Copy = imgFrame2.clone();          // get another copy of frame 2 since we changed the previous frame 2 copy in the processing above

#
		//draw the bounding rectangles
		drawBlobInfoOnImage(blobs, imgFrame2Copy);

		//check if the line is triggered. switch the line to green

		bool lineCrossedOut = checkIfBlobsCrossedTheLineOut(blobs, LinePos, triggerOut);

		if (lineCrossedOut == true) {
			line(imgFrame2Copy, crossingLine[0], crossingLine[1], YELLOW, 2);
		}
		else {
			line(imgFrame2Copy, crossingLine[0], crossingLine[1], RED, 2);
		}

		cout << "---------------------------------------------------------" << endl;

		cout << "Triggers: " << triggerOut << endl;
		cout << "---------------------------------------------------------" << endl;


		//draw the count in the top right corner
		// this is just for the cars moving down
		
		drawtriggerOnImage( triggerOut, imgFrame2Copy);

		imshow("imgFrame2Copy", imgFrame2Copy);
		
		// waitKey(0);                 // uncomment this line to go frame by frame for debugging

		// now we prepare for the next set of frames

		currentFrameBlobs.clear();

		imgFrame1 = imgFrame2.clone();           // move frame 1 up to where frame 2 is
		waitKey(5);

		if ((capVideo.get(CAP_PROP_POS_FRAMES) + 1) < capVideo.get(CAP_PROP_FPS)) {
			capVideo.read(imgFrame2);
		}
		else {
			cout << "video FPS dropped to zero \n";
			break;
		}

		firstFrame = false;
		frameCount++;
		CheckForEscKey = waitKey(1);
	
	}

	if (CheckForEscKey != 27) {               // if the user did not press esc (i.e. we reached the end of the video)
		waitKey(0);                           // hold the windows open to allow the "end of video" message to show
	}

	// note that if the user did press esc, we don't need to hold the windows open, we can simply let the program end which will close the windows
	return(0);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob& currentFrameBlob, vector<Blob>& existingBlobs, int& intIndex) {

	existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
	existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

	existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

	existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
	existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

	existingBlobs[intIndex].blnStillBeingTracked = true;
	existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob& currentFrameBlob, vector<Blob>& existingBlobs) {

	currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

	existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(Point point1, Point point2) {

	int intX = abs(point1.x - point2.x);
	int intY = abs(point1.y - point2.y);
	//pyhtagorean theorem to find the shortest distance between predictions
	return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(Size imageSize, vector< vector< Point> > contours, string strImageName) {
	Mat image(imageSize, CV_8UC3, BLACK);

	drawContours(image, contours, -1, WHITE, -1);

	imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(Size imageSize, vector<Blob> blobs, string strImageName) {

	Mat image(imageSize, CV_8UC3, BLACK);

	vector< vector< Point> > contours;

	for (auto& blob : blobs) {
		if (blob.blnStillBeingTracked == true) {
			contours.push_back(blob.currentContour);
		}
	}

	drawContours(image, contours, -1, WHITE, -1);

	imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineOut(vector<Blob>& blobs, int& LinePos, int& triggerOut) {
	bool lineCrossedOut = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].y < LinePos && blob.centerPositions[currFrameIndex].y >= LinePos) {
				triggerOut++;
				lineCrossedOut = true;
			}
		}

	}

	return lineCrossedOut;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawBlobInfoOnImage(vector<Blob>& blobs, Mat& imgFrame2Copy) {

	for (unsigned int i = 0; i < blobs.size(); i++) {

		if (blobs[i].blnStillBeingTracked == true) {
			rectangle(imgFrame2Copy, blobs[i].currentBoundingRect, RED, 2);

			//uncomment here to display the tracking info on the rectangles
				/*int intFontFace = FONT_HERSHEY_SIMPLEX;
				double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
				int intFontThickness = (int) round(dblFontScale * 1.0);
				putText(imgFrame2Copy,  to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, GREEN, intFontThickness);
				*/
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawtriggerOnImage(int& triggerOut, Mat& imgFrame2Copy) {

	int intFontFace = FONT_HERSHEY_SIMPLEX;
	double dblFontScale = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 300000.0;
	int intFontThickness = (int)round(dblFontScale * 1.5);

	Size textSizeOut = getTextSize(to_string(triggerOut), intFontFace, dblFontScale, intFontThickness, 0);
	Point textOut;

	textOut.x = imgFrame2Copy.cols - 1 - (int)((double)textSizeOut.width * 1.25);
	textOut.y = (int)((double)textSizeOut.height * 1.25);
	putText(imgFrame2Copy, to_string(triggerOut), textOut, intFontFace, dblFontScale, GREEN, intFontThickness);


}


void matchCurrentFrameBlobsToExistingBlobs(vector<Blob>& existingBlobs, vector<Blob>& currentFrameBlobs) {

	for (auto& existingBlob : existingBlobs) {

		existingBlob.blnCurrentMatchFoundOrNewBlob = false;

		existingBlob.predictNextPosition();
	}

	for (auto& currentFrameBlob : currentFrameBlobs) {

		int intIndexOfLeastDistance = 0;
		double dblLeastDistance = 100000.0;

		for (unsigned int i = 0; i < existingBlobs.size(); i++) {

			if (existingBlobs[i].blnStillBeingTracked == true) {

				double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

				if (dblDistance < dblLeastDistance) {
					dblLeastDistance = dblDistance;
					intIndexOfLeastDistance = i;
				}
			}
		}

		if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) {
			addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
		}
		else {
			addNewBlob(currentFrameBlob, existingBlobs);
		}

	}

	for (auto& existingBlob : existingBlobs) {

		if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
			existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
		}

		if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
			existingBlob.blnStillBeingTracked = false;
		}

	}

}
