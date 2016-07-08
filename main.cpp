#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <cstdlib>
#include <iostream>
#include <string>
#include "tcp_ip.h"
#include <boost/filesystem.hpp>
//#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "guiupdater.h"

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

using namespace std;
using namespace boost::filesystem;

void ThreadStart(TCP_IP &obj, char *output)
{
    if (obj.isOpen())
    {
        obj.read(output);
    }
}


void buildWriteCmd(char cmd[], int value)
{

    // integer to char
    char valueToStr[1];
    sprintf(valueToStr,"%d", value);

    // concatenate cmd & val & \r\n (for comm)
    strcat(cmd, strcat(valueToStr, "\r\n"));
}

void startup(GUIupdater *ui)
{

    const char *ip = "1.1.1.1";
    const char *port = "2100";

    // initialise application mode
    int mode = OFFLINE;
    ui->setMode(mode);

    // initialize TCP/IP communication
    TCP_IP conn (ip, port);

    // check connection
    if (conn.isOpen()){

        qDebug() << "Connection OK";

        // connected. in standby mode
        mode = STANDBY;
        ui->setMode(mode);

        void *p;
        char output[DEFAULT_BUFLEN];

        while (true){

            // have the read command within a thread, so the communication is always opened
            boost::thread readingThread(ThreadStart, conn, output);

            if (strcmp(output, "disconnect\r\n") == 0)
            {

                // disconnecting from peer
                //qDebug() << output;

                // send acknowledgment
                conn.write("disconnect:1\r\n");

                // give it some time before disconnecting
                Sleep(500);

                // disconnect from this end
                conn.disconnect();

                break;
            }
            else if (strcmp(output, "!segment\r\n") == 0)
            {

            }
            else if (strcmp(output, "?mode\r\n") == 0)
            {

                char cmd[64] = "mode:";
                buildWriteCmd(cmd, mode);

                // send acknowledgment
                conn.write(cmd);

                qDebug() << output;
                qDebug() << cmd;
            }
            else
            {
                if (strcmp(output, "") == 0) {

                    // make sure we are alive
                    conn.write("alive\r\n");

                    if (!conn.isOpen())
                    {
                        qDebug() << "To disconnect";

                        // disconnect
                        conn.disconnect();

                        // out of the loop
                        break;

                    }

                    //Sleep(1000);
                }


            }

            readingThread.join();
        }

    } else {
        qDebug() << "Connection not OK";
    }

    startup(ui);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // create UI
    MainWindow w;
    w.show();

    // initialization
    w.init();

    // make things working
    // send updater as argument of the thread in order to be able to update the UI from within the thread
    // For some weird reason, QT does not allow the UI to be updated from outside the source class, without using SIGNALS and SLOTS
    boost::thread startupThread(startup, w.updater);

    return a.exec();
}
