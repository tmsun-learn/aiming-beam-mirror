#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QDebug>
#include <iostream>
#include "tcp_ip.h"
#include <boost/filesystem.hpp>
//#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "guiupdater.h"
#include "camera.h"
#include "videopointgrey.h"
#include "videoinput.h"
#include "opencv2/core.hpp"
#include "imageacquisition.h"
#include "threadwrapper.h"
#include "errorhandler.h"
#include "laguerredeconvolution.h"

#define OFFLINE -1
#define STANDBY 0
#define TEST_EXVIVO 1
#define TEST_INVIVO 2
#define ACQUISITION_EXVIVO 3
#define ACQUISITION_INVIVO 4

#define STARTUP 0
#define INIT_CAMERA 1
#define INIT_IRF 2
#define IDLE 3
#define BUSY 4
#define COMPLETE 5
#define SERROR 6
#define ABORT 7

#define TERMINATOR "\r\n"
#define SEPARATOR ":"

using namespace std;
using namespace boost::filesystem;
using namespace cv;

void buildWriteCmd(char cmd[], int value)
{

    // integer to char
    char valueToStr[1];
    sprintf(valueToStr,"%d", value);

    // concatenate cmd & val & \r\n (for comm)
    strcat(cmd, strcat(valueToStr, "\r\n"));
}

char* set_ack (string key, string value)
{
    string cmd = key + SEPARATOR + value + TERMINATOR;

    char cmd_[1024];
    strncpy(cmd_, cmd.c_str(), sizeof(cmd_));
    cmd_[sizeof(cmd_) - 1] = 0;

    return cmd_;
}

void startup(GUIupdater *ui)
{

    const char *ip = "1.1.1.1";
    const char *port = "2100";
    bool init = true;

    // other vars
    string subject;     // case ID no.
    bool aiming_beam_correction = false;    // automatic correction of the aiming beam
    int aiming_beam_value = 0;  // aiming beam power (0 to 100)
    bool invivo = false;    // ex vivo false, in vivo true
    float time_resolution = 0;  // digitizer time resolution, value in ns
    int data_length = 0;    // length of the fluorescence data in each channel?
    int irf_length = 0;     // length of the IRF in each channel

    // lifetime boundaries & other display parameters
    bool lt_auto = true; // automatic scale
    int lt_min = 1;     // min limit
    int lt_max = 6;     // max limit
    int channel = 2;    // channel to be displayed
    int ansi = 5;       // ansi limit, as the maximum number of frames that a single pixel can be imaged
    float threshold = 0.96;    // cross-correlation threshold

    // image acquisition settings
    bool focus = false;     // if true, manual focus is on and camera is streaming
    bool acquire = false;   // if true, enter in acquisition mode

    // initialise application mode
    int mode = OFFLINE;
    ui->setMode(mode);

    // initialize image acquisition object
    imageAcquisition *acq = new imageAcquisition();



    while (!acq->ready)
    {
        ui->throwError("Camera not detected");

        // acknowledge error
        ui->setError(true);

        // shutdown camera and cleanup
        acq->shutdownCamera();

        // wait
        Sleep(1000);

        // reconnect to camera
        acq->startupCamera(channel, threshold);


    }

    ui->setError(false);

    // initialize TCP/IP communication
    TCP_IP conn (ip, port);

    // make sure we are alive
    conn.write("alive\r\n");

    // check connection
    if (conn.isOpen()){

        // connected. in standby mode
        mode = STANDBY;
        ui->setMode(mode);

        // acknowledge we are ready to go
        ui->setReady(true);

        // initialize vars to manage communications
        string output;
        string key;
        string value;
        int len;

        // variables to hold IRFs & data
        vector<double> iIRF_CH1;
        vector<double> iIRF_CH2;
        vector<double> iIRF_CH3;
        vector<double> iIRF_CH4;
        vector<double> data_CH1;
        vector<double> data_CH2;
        vector<double> data_CH3;
        vector<double> data_CH4;
        vector<double> tempdata;

        int idx = 0;
        double ch1_tau = 0.0;
        double ch2_tau = 0.0;
        double ch3_tau = 0.0;
        double ch4_tau = 0.0;

        acq->thread = true; // control acquisition thread

        // start acquisition thread, to run indefinitely
        boost::thread AcquisitionThread(ThreadWrapper::StartAcquire, acq);
        while (true){

            // have the read command within a thread, so the communication is always opened
            boost::thread readingThread(ThreadWrapper::StartRead, conn, &output, &len, &tempdata);

            // strip output \r\n
            output = output.substr(0, len - 2);

            // get key and value from output
            size_t f = output.find(":");

            if (f != std::string::npos)
            {
                // get key (string before :)
                key = output.substr(0, f);

                // get value (string after :), always as double or int
                //  +1 to account for the :
                value = output.substr(f + 1, len - f + 1);

            } else {
                // set defaults
                key = output;
                value = "";
            }

            // list of commands
            // disconnect from peer
            if (key.compare("disconnect") == 0)
            {


                // disconnecting from peer
                qDebug() << "disconnecting";

                // send acknowledgment
                conn.write("disconnect:1\r\n");

                // give it some time before disconnecting
                Sleep(500);

                // disconnect from this end
                conn.disconnect();

                // wait so that master can close connection
                Sleep(500);

                break;
            }



            // setting subject id
            else if (key.compare("!subject") == 0)
            {

                subject = value;

                // send acknowledgment
                conn.write("test\r\n");
                //conn.write(set_ack(key, subject));

            }

            // aiming beam correction on/off?
            else if (key.compare("!beam_adjust") == 0)
            {

                // aiming beam correction?
                aiming_beam_correction = (value.compare("1") == 0) ? true : false;
                conn.write(set_ack(key, value));
            }

            // aiming beam value
            else if (key.compare("!beam_value") == 0)
            {

                // aiming beam value
                aiming_beam_value = stoi(value);
                conn.write(set_ack(key, value));
            }

            // digitizer time resolution
            else if (key.compare("!resolution") == 0)
            {

                time_resolution = stod(value);

                // send acknowledgment
                conn.write(set_ack(key, value));

                //qDebug() << time_resolution;

            }

            // length of the fluorescence data
            else if (key.compare("!data_length") == 0)
            {

                data_length = stoi(value);

                // send acknowledgment
                conn.write(set_ack(key, value));

            }

            // IRF length
            else if (key.compare("!irf_length") == 0)
            {

                irf_length = stoi(value);

                // send acknowledgment
                conn.write(set_ack(key, value));

            }

            // in vivo/ex vivo modes
            else if (key.compare("!invivo") == 0)
            {

                // aiming beam correction?
                invivo = (value.compare("1") == 0) ? true : false;
                conn.write(set_ack(key, value));

            }

            // manual focus
            else if (key.compare("!focus") == 0)
            {
                focus = (value.compare("1") == 0) ? true : false;
                conn.write(set_ack(key, value));
            }

            // acquisition?
            else if (key.compare("!acquire") == 0)
            {
                acquire = (value.compare("1") == 0) ? true : false;
                conn.write(set_ack(key, value));

            }

            // automatic lifetime boundaries
            else if (key.compare("!autocolor") == 0)
            {
                lt_auto = (value.compare("1") == 0) ? true : false;
                conn.write(set_ack(key, value));
            }

            // lifetime boundaries
            else if(key.compare("!color") == 0)
            {

                // get key and value from output
                size_t s = value.find(";");

                if (s != std::string::npos)
                {
                    // get min lifetime
                    lt_min = stoi(value.substr(0, s));

                    // get max lifetime
                    lt_max = stoi(value.substr(s + 1, value.length() - f + 1));

                }

                conn.write(set_ack(key, value));

            }

            // channel to be displayed
            else if (key.compare("!channel") == 0)
            {
                // string to integer
                channel = stoi(value);

                // set acquisition channel
                acq->channel = channel;

                // acknowledgment
                conn.write(set_ack(key, value));
            }

            // channel to be displayed
            else if (key.compare("!ansi") == 0)
            {
                ansi = stoi(value);
                conn.write(set_ack(key, value));
            }

            // correlation threshold
            else if (key.compare("!threshold") == 0)
            {
                // string to double
                threshold = stod(value);

                // set cross-correlation threshold
                acq->threshold = threshold;

                //qDebug() << threshold;

                conn.write(set_ack(key, value));
            }

            // query mode
            else if (key.compare("?mode") == 0)
            {

                char cmd[64] = "mode:";
                buildWriteCmd(cmd, mode);

                // send acknowledgment
                conn.write(cmd);

            }

            // channel 1 IRF (instrument response function)
            else if (key.compare("&irf1") == 0)
            {
                iIRF_CH1 = tempdata;
                qDebug() << iIRF_CH1.size();
                conn.write(set_ack(key, "1"));

            }

            // channel 2 IRF (instrument response function)
            else if (key.compare("&irf2") == 0)
            {
                iIRF_CH2 = tempdata;
                qDebug() << iIRF_CH2.size();
                conn.write(set_ack(key, "1"));

            }

            // channel 3 IRF (instrument response function)
            else if (key.compare("&irf3") == 0)
            {
                iIRF_CH3 = tempdata;
                qDebug() << iIRF_CH3.size();
                conn.write(set_ack(key, "1"));

            }

            // channel 4 IRF (instrument response function)
            else if (key.compare("&irf4") == 0)
            {
                iIRF_CH4 = tempdata;
                qDebug() << iIRF_CH4.size();
                conn.write(set_ack(key, "1"));

            }

            // channel 1 data
            else if (key.compare("%dat1") == 0)
            {
                data_CH1 = tempdata;
                //qDebug() << "channel 1";
               // qDebug() << len;
                conn.write(set_ack(key, "1"));
            }

            // channel 2 data
            else if (key.compare("%dat2") == 0)
            {
                data_CH2 = tempdata;
                //qDebug() << "channel 2";
                conn.write(set_ack(key, "2"));
            }

            // channel 3 data
            else if (key.compare("%dat3") == 0)
            {
                data_CH3 = tempdata;
                //qDebug() << "channel 3";
                conn.write(set_ack(key, "3"));
            }

            // channel 4 data
            else if (key.compare("%dat4") == 0)
            {
                data_CH4 = tempdata;
                // do laguerre here
                //qDebug() << "channel 4";
                conn.write(set_ack(key, "4"));
            }

            // data point id
            else if (key.compare("!idx") == 0)
            {
                idx = stoi(value);
                //qDebug() << idx;
                conn.write(set_ack(key, value));
            }

            else
            {

                // something unexpected was received. Are we still connected?
                if (!init)
                {
                    qDebug() << "being alive is good...";

                    // make sure we are still connected
                    conn.write("alive\r\n");

                    if (!conn.isOpen())
                    {
                        qDebug() << "To disconnect";

                        // disconnect
                        conn.disconnect();

                        // out of the loop
                        break;
                    }
                }

                // the buffer always needs to cleared upon initialization
                // Therefore, we use this var not to allow the command alive to be sent during init
                init = false;

            }

            // set acquisition loop condition
            acq->inAcquisition = acquire;   // differentiate between focusing and acquisition
            acq->ctrl = focus || acquire;

            // check current working mode
            mode = (focus) ?
                        ((invivo) ? TEST_INVIVO : TEST_EXVIVO)
                      :
                        (acquire) ?
                            ((invivo) ? ACQUISITION_INVIVO : ACQUISITION_EXVIVO)
                          :
                            STANDBY;
            ui->setMode(mode);

            readingThread.join();

        }

        // make sure the thread is not working forever
        acq->thread = false;
        AcquisitionThread.join();
        AcquisitionThread.detach();

    } else {
        qDebug() << "Connection not OK";
    }

    // restart the application instead of having a recursive function. this clears up memory as well
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    //startup(ui);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // create UI
    MainWindow w;
    w.show();

    // initialization
    w.ready(false);
    w.error(false);
    w.init();

    // make things working
    // send updater as argument of the thread in order to be able to update the UI from within the thread
    // For some weird reason, QT does not allow the UI to be updated from outside the source class, without using SIGNALS and SLOTS
    boost::thread startupThread(startup, w.updater);

    return a.exec();
}