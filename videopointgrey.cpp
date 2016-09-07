#include "videopointgrey.h"
#include "FlyCapture2.h"
#include <iostream>
#include <QDebug>
#include "errorhandler.h"

using namespace FlyCapture2;
using namespace std;

void PrintError( Error error )
{
    error.PrintErrorTrace();
}


VideoPointGrey::VideoPointGrey()
{

    // Connect to a camera
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        //PrintError(error);
        return;
        //exit(-1); //Replace the exit commands
    }

    if ( numCameras < 1 )
    {
        //cout << "No camera detected." << endl;
        return;
    }

    error = busMgr.GetCameraFromIndex(0, &guid);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return;
    }

    error = cam.Connect(&guid);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return;
    }

    error = cam.GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return;
    }
    error = cam.StartCapture();
    if ( error == PGRERROR_ISOCH_BANDWIDTH_EXCEEDED )
    {
        std::cout << "Bandwidth exceeded" << std::endl;
        return;
    }

    connected = true;

}

bool VideoPointGrey::isConnected()
{
    return connected;
}

cv::Mat VideoPointGrey::getNextFrame( )
{
    // Get the image

    error = cam.RetrieveBuffer( &rawImage );

    if ( error != PGRERROR_OK )
    {
        std::cout << "capture error" << std::endl;
        exit(-1);
    }

    // convert to rgb
    rawImage.Convert( FlyCapture2::PIXEL_FORMAT_BGR, &rgbImage );

    // convert to OpenCV Mat
    unsigned int rowBytes = (double)rgbImage.GetReceivedDataSize()/(double)rgbImage.GetRows();
    cv::Mat image = cv::Mat(rgbImage.GetRows(), rgbImage.GetCols(), CV_8UC3, rgbImage.GetData(),rowBytes);
    return image.clone();
}


bool VideoPointGrey::lastFrame()
{
    return false;
}

int VideoPointGrey::getNumberOfFrames()
{
    return 0;
}

void VideoPointGrey::disconnect()
{
    cam.Disconnect();
    connected = false;
}
