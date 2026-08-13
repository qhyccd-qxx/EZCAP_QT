#include "gpsTool.h"
#include "ui_gpstool.h"
#include "dllqhyccd.h"
#include "myStruct.h"
#include "outputdebug.h"
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QTextCodec>
#endif

gpsTool *gpsTool_dialog;
extern qhyccd_handle *camhandle;
extern QMutex gpsMutex;

gpsTool::gpsTool(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::gpsTool),
    gpsTimeValid(false),
    gpsSyncEnabled(false)
{
    ui->setupUi(this);

    QTimer *syncTimer = new QTimer(this);
    syncTimer->setInterval(1000);
    connect(syncTimer, SIGNAL(timeout()), this, SLOT(syncSystemTimeOnTimer()));
    syncTimer->start();
}

gpsTool::~gpsTool()
{
    delete ui;
}

typedef struct
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
    uint16_t week;
}drive_time,*pdrive_time;

drive_time struct_time = //初始化时间
{
    .year = 1995,
    .month = 10,
    .day = 10,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

bool isLeapYear( int year )
{
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))
        return true;
    return false;
}

//获取世界标准时间，转换成北京时间需要加上 8 小时
int get_UTC(unsigned long second, pdrive_time UTC)
{
    const char Leap_Year_day[2][12] = { {31,28,31,30,31,30,31,31,30,31,30,31},{31,29,31,30,31,30,31,31,30,31,30,31} };
    int Leap_Year = 0;
    int month_day = 0;

    Leap_Year = isLeapYear(struct_time.year);
    month_day = Leap_Year_day[Leap_Year][struct_time.month-1];

    UTC->year = struct_time.year;
    UTC->month = struct_time.month;
    UTC->day = struct_time.day;
    UTC->hour = struct_time.hour +(second / 3600 % 24);
    UTC->min = struct_time.min+ (second / 60 % 60);
    UTC->sec = struct_time.sec +(second % 60);

    uint16_t count_days = second / 86400;

    if(UTC->sec >=60)
    {
        UTC->sec = UTC->sec%60;
        (UTC->min) ++;
    }

    if(UTC->min >=60)
    {
        UTC->min = UTC->min%60;
        (UTC->hour) ++;
    }

    if(UTC->hour >=24)
    {
        UTC->hour = UTC->hour%24;
        (count_days) ++;
    }

    for(int i = 0 ; i < count_days; i++ )
    {
        Leap_Year = isLeapYear(UTC->year);
        month_day = Leap_Year_day[Leap_Year][(UTC->month)-1];
        (UTC->day) ++;
        if((UTC->day) > month_day)
        {
            (UTC->day) = 1;
            (UTC->month) ++;
            if((UTC->month) > 12)
            {
                (UTC->month) = 1;
                (UTC->year) ++;
                if( ( (UTC->year) - (struct_time.year) ) >100)
                    return -1;
            }
        }
    }

    return 0 ;
}

void gpsTool::on_comBox_GPSOnOff_currentTextChanged(const QString &arg1)
{
    if(arg1 == "") return;

    uint32_t ret = QHYCCD_ERROR;

    if(arg1 == "ON")
    {
        ret = libqhyccd->SetQHYCCDParam(camhandle, CAM_GPS, 1.0);
        ix.GPS = true;
    }
    else
    {
        ret = libqhyccd->SetQHYCCDParam(camhandle, CAM_GPS, 0.0);
        ix.GPS = false;
    }



    libqhyccd->SetQHYCCDGPSLedCalMode(camhandle, 1);

    uint32_t vPosA, vPosB;
    if(ix.CamID.contains("QHY9A_992") || ix.CamID.contains("QHY992_"))
    {
        if(ix.Bits == 8)
        {
            vPosB = 1136;
            vPosA = 620;
        }
        else
        {
            vPosB = 1367;
            vPosA = 980;
        }

        ui->spBox_GPSStart8->blockSignals(true);
        ui->spBox_GPSStart7->blockSignals(true);
        ui->spBox_GPSStart6->blockSignals(true);
        ui->spBox_GPSStart5->blockSignals(true);
        ui->spBox_GPSStart4->blockSignals(true);
        ui->spBox_GPSStart3->blockSignals(true);
        ui->spBox_GPSStart2->blockSignals(true);
        ui->spBox_GPSStart1->blockSignals(true);
        ui->spBox_GPSStart8->setValue(0);
        ui->spBox_GPSStart7->setValue(0);
        ui->spBox_GPSStart6->setValue(0);
        ui->spBox_GPSStart5->setValue(0);
        ui->spBox_GPSStart4->setValue(vPosB / 1000);
        ui->spBox_GPSStart3->setValue((vPosB % 1000) / 100);
        ui->spBox_GPSStart2->setValue((vPosB % 100) / 10);
        ui->spBox_GPSStart1->setValue(vPosB % 10);
        ui->spBox_GPSStart8->blockSignals(false);
        ui->spBox_GPSStart7->blockSignals(false);
        ui->spBox_GPSStart6->blockSignals(false);
        ui->spBox_GPSStart5->blockSignals(false);
        ui->spBox_GPSStart4->blockSignals(false);
        ui->spBox_GPSStart3->blockSignals(false);
        ui->spBox_GPSStart2->blockSignals(false);
        ui->spBox_GPSStart1->blockSignals(false);

        ui->spBox_GPSEnd8->blockSignals(true);
        ui->spBox_GPSEnd7->blockSignals(true);
        ui->spBox_GPSEnd6->blockSignals(true);
        ui->spBox_GPSEnd5->blockSignals(true);
        ui->spBox_GPSEnd4->blockSignals(true);
        ui->spBox_GPSEnd3->blockSignals(true);
        ui->spBox_GPSEnd2->blockSignals(true);
        ui->spBox_GPSEnd1->blockSignals(true);
        ui->spBox_GPSEnd8->setValue(0);
        ui->spBox_GPSEnd7->setValue(0);
        ui->spBox_GPSEnd6->setValue(0);
        ui->spBox_GPSEnd5->setValue(0);
        ui->spBox_GPSEnd4->setValue(vPosA / 1000);
        ui->spBox_GPSEnd3->setValue((vPosA % 1000) / 100);
        ui->spBox_GPSEnd2->setValue((vPosA % 100) / 10);
        ui->spBox_GPSEnd1->setValue(vPosA % 10);
        ui->spBox_GPSEnd8->blockSignals(false);
        ui->spBox_GPSEnd7->blockSignals(false);
        ui->spBox_GPSEnd6->blockSignals(false);
        ui->spBox_GPSEnd5->blockSignals(false);
        ui->spBox_GPSEnd4->blockSignals(false);
        ui->spBox_GPSEnd3->blockSignals(false);
        ui->spBox_GPSEnd2->blockSignals(false);
        ui->spBox_GPSEnd1->blockSignals(false);
    }
    else
    {
        vPosB = static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
                static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
                static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
                static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
                static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
                static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
                static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
                static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
                static_cast<uint32_t>(ui->spBox_GPSStart1->value());
        vPosA = static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
                static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
                static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
                static_cast<uint32_t>(ui->spBox_GPSEnd1->value());
    }

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
    {
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, vPosB, 40);
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, vPosA, 40);
    }
    else
    {
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, vPosB, 40);
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, vPosA, 40);
    }

    libqhyccd->SetQHYCCDGPSLedCalMode(camhandle, 0);
}

void gpsTool::on_comBox_GPSFreq_currentTextChanged(const QString &arg1)
{
    if(arg1 == "") return;

    if(arg1 == "ON")
    {
        libqhyccd->SetQHYCCDGPSVCOXFreq(camhandle, 2000);
    }
}

void gpsTool::on_comBox_LEDCalMode_currentTextChanged(const QString &arg1)
{
    if(arg1 == "") return;

    if(arg1 == "ON")
        libqhyccd->SetQHYCCDGPSLedCalMode(camhandle, 1);
    else
        libqhyccd->SetQHYCCDGPSLedCalMode(camhandle, 0);
}

void gpsTool::on_comBox_GPSSlaveOnOff_currentTextChanged(const QString &arg1)
{
    if(arg1 == "") return;

    if(arg1 == "ON")
    {
        if(!ix.CamID.contains("QHY992_2") && !ix.CamID.contains("QHY992_3") && !ix.CamID.contains("QHY992_4") &&
           !ix.CamID.contains("QHY992_5") && !ix.CamID.contains("QHY992_6") && !ix.CamID.contains("QHY992_7") &&
           !ix.CamID.contains("QHY992_8") && !ix.CamID.contains("QHY992_9"))
            libqhyccd->SetQHYCCDGPSMasterSlave(camhandle, 1);

        if(ix.CamID.contains("QHY992_1"))
        {
            uint8_t data[16] = { 0 };
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 150, 3);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 151, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0,  58, 5);
            libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, 0, 158, 1, data);
            libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, 1,  18, 1, data);
        }
        else if(ix.CamID.contains("QHY992_2") || ix.CamID.contains("QHY992_3") || ix.CamID.contains("QHY992_4") ||
                ix.CamID.contains("QHY992_5") || ix.CamID.contains("QHY992_6") || ix.CamID.contains("QHY992_7") ||
                ix.CamID.contains("QHY992_8") || ix.CamID.contains("QHY992_9"))
        {
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 150, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 151, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0,  58, 5);
        }
    }
    else
    {
        if(!ix.CamID.contains("QHY992_2") && !ix.CamID.contains("QHY992_3") && !ix.CamID.contains("QHY992_4") &&
            !ix.CamID.contains("QHY992_5") && !ix.CamID.contains("QHY992_6") && !ix.CamID.contains("QHY992_7") &&
            !ix.CamID.contains("QHY992_8") && !ix.CamID.contains("QHY992_9"))
            libqhyccd->SetQHYCCDGPSMasterSlave(camhandle, 0);

        if(ix.CamID.contains("QHY992_1"))
        {
            uint8_t data[16] = { 0 };
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 150, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 151, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0,  58, 5);
            libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, 1, 158, 1, data);
            libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, 1,  18, 1, data);
        }
        else if(ix.CamID.contains("QHY992_2") || ix.CamID.contains("QHY992_3") || ix.CamID.contains("QHY992_4") ||
                 ix.CamID.contains("QHY992_5") || ix.CamID.contains("QHY992_6") || ix.CamID.contains("QHY992_7") ||
                 ix.CamID.contains("QHY992_8") || ix.CamID.contains("QHY992_9"))
        {
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 150, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0, 151, 1);
            libqhyccd->SetQHYCCDWriteFPGA(camhandle, 0,  58, 5);
        }
    }
}

void gpsTool::on_spBox_GPSStart1_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart2_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart3_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart4_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart5_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart6_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart7_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart8_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSStart9_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSStart9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSStart6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSStart5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSStart4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSStart3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSStart2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSStart1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSB(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd1_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd2_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd3_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd4_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd5_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd6_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd7_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd8_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

void gpsTool::on_spBox_GPSEnd9_valueChanged(int arg1)
{
    uint32_t value =
            static_cast<uint32_t>(ui->spBox_GPSEnd9->value()) * 100000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd8->value()) * 10000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd7->value()) * 1000000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd6->value()) * 100000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd5->value()) * 10000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd4->value()) * 1000 +
            static_cast<uint32_t>(ui->spBox_GPSEnd3->value()) * 100 +
            static_cast<uint32_t>(ui->spBox_GPSEnd2->value()) * 10 +
            static_cast<uint32_t>(ui->spBox_GPSEnd1->value());

    if(ui->comBox_GPSSlaveOnOff->currentText() == "ON")
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 1, value, 40);
    else
        libqhyccd->SetQHYCCDGPSPOSA(camhandle, 0, value, 40);
}

int Date2Jd(int y, int m, int d) {
    int ky = y;
    int km = m;
    int B = -2;

    if (km <= 2) {
        ky = ky - 1;
        km = km + 12;
    }
    if (ky > 1582 || (ky == 1582 && (km > 10 || (km == 10 && d >= 15)))) {
        B = ky / 400 - ky / 100;
    }

    double JD = static_cast<int>(365.25 * ky) + static_cast<int>(30.6001 * (km + 1)) + B + 1720996.5 + d;

    return static_cast<int>(JD);
}

double gpsTool::Date2Js1995(int y, int M, int d, int h, int m, int s)
{
    double Jd;
    double Jds;
    double Jd1995;
    double Js1995;

    Jd = Date2Jd(y, M, d);
    Jds = Jd + static_cast<double>(h) / 24 + static_cast<double>(m) / 60 / 24 + static_cast<double>(s) / 60 / 60 / 24;
    Jd1995 = Jd - 2450000; // Zero time is 19951009
    Js1995 = Jd1995 * 24 * 3600 + h * 3600 + m * 60 + s;

    return Js1995;
}

void gpsTool::on_pBtn_GPSSetUTC_clicked()
{
    double target = Date2Js1995(ui->spinBox_GPSYear->value(),
                               ui->spinBox_GPSMonth->value(),
                               ui->spinBox_GPSDay->value(),
                               ui->spinBox_GPSHour->value(),
                               ui->spinBox_GPSMinute->value(),
                               ui->spinBox_GPSSecond->value());
    uint32_t expunit = 1000;
    if(ui->comBox_GPSExpUnit->currentText() == "us")      expunit = 1;
    else if(ui->comBox_GPSExpUnit->currentText() == "ms") expunit = 1000;
    else if(ui->comBox_GPSExpUnit->currentText() == "s")  expunit = 1000000;
    uint32_t ExpTime = static_cast<uint32_t>(ui->spBox_GPSExpTime->value()) * expunit;

    libqhyccd->SetQHYCCDGPSSlaveModeParameter(camhandle, target, 0, 0, 0, ExpTime);
}

QString removeInvalidCharacters(const QString& str) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStringDecoder codec = QStringDecoder("UTF-8");
    if(codec.hasError()) {
#else
    QTextCodec* codec = QTextCodec::codecForName("UTF-8"); // 指定字符编码，这里使用UTF-8
    if (!codec) {
#endif
        qDebug() << "不支持的字符编码";
        return str;
    }

    QString cleanedStr;
    for (int i = 0; i < str.length(); ++i) {
        QChar currentChar = str.at(i);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QByteArray charBytes = QStringEncoder("UTF-8")(currentChar);
        QString decodedChar = codec(charBytes);
#else
        QByteArray charBytes = codec->fromUnicode(currentChar);
        QString decodedChar = codec->toUnicode(charBytes);
#endif
        if (currentChar == decodedChar) {
            cleanedStr.append(currentChar);
        }
    }

    return cleanedStr;
}

void gpsTool::updateGPSInfo()
{
    unsigned char gpsData[1024] = { 0 };
    {
        QMutexLocker locker(&gpsMutex);
        if(!ix.ImgData_GPS)
        {
            gpsTimeValid = false;
            ui->label_GPSSyncStatus->setText("sync_status: no GPS data");
            return;
        }
        memcpy(gpsData, ix.ImgData_GPS, sizeof(gpsData));
    }

    int now_flag = (gpsData[33] / 16) % 4;
    int pps = 256 * 256 * gpsData[41] + 256 * gpsData[42] + gpsData[43];
    long seqNumber = 256 * 256 * 256 * gpsData[0] + 256 * 256 * gpsData[1] + 256 * gpsData[2] + gpsData[3];
    int width = 256 * gpsData[5] + gpsData[6];
    int height = 256 * gpsData[7] + gpsData[8];
    long temp = 256 * 256 * 256 * gpsData[9] + 256 * 256 * gpsData[10] + 256 * gpsData[11] + gpsData[12];
    int south = temp > 1000000000;
    int deg = (temp % 1000000000) / 10000000;
    int min = (temp % 10000000) / 100000;
    double fractMin = (temp % 100000) / 100000.0;
    double latitude = (deg + (min + fractMin) / 60.0) * (south==0?1:-1);
    temp = 256 * 256 * 256 * gpsData[13] + 256 * 256 * gpsData[14] + 256 * gpsData[15] + gpsData[16];
    int west = temp > 1000000000;
    deg = (temp % 1000000000) / 1000000;
    min = (temp % 1000000) / 10000;
    fractMin = (temp % 10000) / 10000.0;
    double longitude = (deg + (min + fractMin) / 60.0) * (west==0?1:-1);
    unsigned long start_sec = 256 * 256 * 256 * gpsData[18] + 256 * 256 * gpsData[19] + 256 * gpsData[20] + gpsData[21];
    unsigned long start_us = (256 * 256 * gpsData[22] + 256 * gpsData[23] + gpsData[24]) / 10;
    unsigned long end_sec = 256 * 256 * 256 * gpsData[26] + 256 * 256 * gpsData[27] + 256 * gpsData[28] + gpsData[29];
    unsigned long end_us = (256 * 256 * gpsData[30] + 256 * gpsData[31] + gpsData[32]) / 10;
    unsigned long now_sec = 256 * 256 * 256 * gpsData[34] + 256 * 256 * gpsData[35] + 256 * gpsData[36] + gpsData[37];
    unsigned long now_us = (256 * 256 * gpsData[38] + 256 * gpsData[39] + gpsData[40]) / 10;
    unsigned long exposure = (unsigned long)(((long)end_sec - (long)start_sec) * 1000 * 1000 + ((long)end_us - (long)start_us));
    unsigned long elevation = 0;
    int gnss = 0;
    int antenna = 0;
    if(ix.CamID.contains("QHY990") || ix.CamID.contains("QHY991"))
    {
        elevation = gpsData[44] * 256 * 256 + gpsData[45] * 256 + gpsData[46];
        antenna = gpsData[47] % 10;
        gnss = gpsData[47] / 10;
    }

    drive_time UTC_start_sec, UTC_end_sec, UTC_now_sec;
    get_UTC(start_sec, &UTC_start_sec);
    get_UTC(end_sec,   &UTC_end_sec);
    get_UTC(now_sec,   &UTC_now_sec);

    ui->label_GPSInfoFlag->setText(     "now_flag   : " + QString::number(now_flag));
    ui->label_GPSInfoPPS->setText(      "pps        : " + QString::number(pps));
    ui->label_GPSInfoSeq->setText(      "seqNumber  : " + QString::number(seqNumber));
    ui->label_GPSInfoWidth->setText(    "width      : " + QString::number(width));
    ui->label_GPSInfoHeight->setText(   "height     : " + QString::number(height));
    ui->label_GPSInfoLatitude->setText( "latitude   : " + QString::number(latitude, 'f', 6));
    ui->label_GPSInfoLongitude->setText("longitude  : " + QString::number(longitude, 'f', 6));
    ui->label_GPSInfoElevation->setText("elevation  : " + QString::number(elevation));
    ui->label_GPSInfoAntenna->setText(  "antenna    : " + QString::number(antenna));
    ui->label_GPSInfoGnss->setText(     "gnss       : " + QString::number(gnss));
    ui->label_GPSInfoExposure->setText( "exposure   : " + QString::number(exposure, 'f', 1));
    ui->label_GPSInfoStartsec->setText( "start_sec  : " + QString::number(UTC_start_sec.year) + "-" +
                                                          QString::number(UTC_start_sec.month) + "-" +
                                                          QString::number(UTC_start_sec.day) + "_" +
                                                          QString::number(UTC_start_sec.hour) + "-" +
                                                          QString::number(UTC_start_sec.min) + "-" +
                                                          QString::number(UTC_start_sec.sec));
    ui->label_GPSInfoStartus->setText(  "start_us   : " + QString::number(start_us, 'f', 1));
    ui->label_GPSInfoEndsec->setText(   "end_sec    : " + QString::number(UTC_end_sec.year) + "-" +
                                                          QString::number(UTC_end_sec.month) + "-" +
                                                          QString::number(UTC_end_sec.day) + "_" +
                                                          QString::number(UTC_end_sec.hour) + "-" +
                                                          QString::number(UTC_end_sec.min) + "-" +
                                                          QString::number(UTC_end_sec.sec));
    ui->label_GPSInfoEndus->setText(    "end_us     : " + QString::number(end_us, 'f', 1));
    ui->label_GPSInfoNowsec->setText(   "now_sec    : " + QString::number(UTC_now_sec.year) + "-" +
                                                          QString::number(UTC_now_sec.month) + "-" +
                                                          QString::number(UTC_now_sec.day) + "_" +
                                                          QString::number(UTC_now_sec.hour) + "-" +
                                                          QString::number(UTC_now_sec.min) + "-" +
                                                          QString::number(UTC_now_sec.sec));
    ui->label_GPSInfoNowus->setText(    "now_us     : " + QString::number(now_us, 'f', 1));
    ui->label_GPSInfoLocalTime->setText("local_time : " + ix.GPS_LocalTime);

    gpsUtcTime = QDateTime(QDate(1995, 10, 10), QTime(0, 0), Qt::UTC)
                     .addSecs(now_sec)
                     .addMSecs(now_us / 1000);
    gpsTimeValid = now_flag == 3 && now_sec > 0 && now_us < 1000000 &&
                   pps > 0 && pps <= 10000500 && gpsUtcTime.isValid() &&
                   gpsUtcTime.date().year() >= 2020 && gpsUtcTime.date().year() <= 2100;
    gpsFrameAge.restart();
    ui->label_GPSUtcTime->setText("gps_utc    : " + gpsUtcTime.toString("yyyy-MM-dd HH:mm:ss.zzz"));
    if(!gpsTimeValid)
        ui->label_GPSSyncStatus->setText("sync_status: GPS invalid");
    else if(!gpsSyncEnabled)
        ui->label_GPSSyncStatus->setText("sync_status: ready");

    if(gpsSyncEnabled && ix.fps >= 1.0)
        syncSystemTimeToGPS();

    //decode the GPS_RAW head( 0X11 22 33 66)
    int i;
    int rawHeadPosition=0;
    for (i = 34; i < 1021; i++)
    {
        if(gpsData[i]==0x11)
        {
            if(gpsData[i+1]==0x22 && gpsData[i+2]==0x33 && gpsData[i+3]==0x66)
            {
                rawHeadPosition=i;
            }
        }
    }
    int rawTailPosition=0;
    for (i = 34; i < 1021; i++)
    {
        if(gpsData[i]==0xee)
        {
            if(gpsData[i+1]==0x33 && gpsData[i+2]==0xcc && gpsData[i+3]==0x44/* &&
               ix.ImgDataGPS[i+4]==0xee &&
               ix.ImgDataGPS[i+5]==0x33 && ix.ImgDataGPS[i+6]==0xcc && ix.ImgDataGPS[i+7]==0x44*/ )
            {
                rawTailPosition=i;
            }
        }
    }

    //get the raw data length   (raw head position +4)
    int GPS_RAW_LENGTH;
    GPS_RAW_LENGTH = rawHeadPosition == 0 ? 0 :
                     gpsData[rawHeadPosition+4]*256*256*256 +
                     gpsData[rawHeadPosition+5]*256*256 +
                     gpsData[rawHeadPosition+6]*256 +
                     gpsData[rawHeadPosition+7];
//    printf("GPS_RAW_LENGTH = %d\n", GPS_RAW_LENGTH);

    char rawstr[1024] = { 0 };//, rawstr_t[1024];
//    int pos = 0;
    if(rawHeadPosition+8+GPS_RAW_LENGTH<1024)
    {
        //get the raw data (raw head position +8)
        for (int j = 0; j < GPS_RAW_LENGTH; j++)
        {
          rawstr[j] = gpsData[j+rawHeadPosition+8];
//          if(rawstr[j] >= 0 && rawstr[j] <= 32 || rawstr[j] == 127)
//          {
//              printf("j = %d\n", j);
//              pos = j;
//          }
        }

//        memcpy(rawstr_t, rawstr, pos);
    }
    QString gpsRawData(rawstr);
    QString show = removeInvalidCharacters(gpsRawData);
    ui->label_GPSRawData->setText("Raw Data  : \n" + show);
}

void gpsTool::on_pBtn_GPSSyncSystemTime_clicked()
{
    if(gpsSyncEnabled)
    {
        gpsSyncEnabled = false;
        ui->pBtn_GPSSyncSystemTime->setText("Start system time sync");
        ui->label_GPSSyncStatus->setText("sync_status: stopped");
        DBGOPT_INFO("GPS system time synchronization stopped");
        return;
    }

    if(!gpsTimeValid || !gpsFrameAge.isValid() || gpsFrameAge.elapsed() > 5000)
    {
        ui->label_GPSSyncStatus->setText("sync_status: no valid GPS time");
        DBGOPT_WARNING("GPS system time synchronization rejected: invalid or stale GPS time");
        return;
    }

    gpsSyncEnabled = true;
    ui->pBtn_GPSSyncSystemTime->setText("Stop system time sync");
    DBGOPT_INFO("GPS system time synchronization started");
    syncSystemTimeToGPS();
}

void gpsTool::syncSystemTimeToGPS()
{
    if(!gpsSyncEnabled)
        return;

    if(!gpsTimeValid || !gpsFrameAge.isValid() || gpsFrameAge.elapsed() > 5000)
    {
        ui->label_GPSSyncStatus->setText("sync_status: waiting for valid GPS");
        return;
    }

    const QDateTime estimatedGpsUtc = gpsUtcTime.addMSecs(gpsFrameAge.elapsed());
    const qint64 differenceMs = QDateTime::currentDateTimeUtc().msecsTo(estimatedGpsUtc);
    ui->label_GPSTimeDifference->setText("time_diff  : " + QString::number(differenceMs) + " ms");

    if(qAbs(differenceMs) <= 20)
    {
        ui->label_GPSSyncStatus->setText("sync_status: synchronized");
        return;
    }

#ifdef Q_OS_WIN
    const QDate date = estimatedGpsUtc.date();
    const QTime time = estimatedGpsUtc.time();
    SYSTEMTIME systemTime;
    systemTime.wYear = static_cast<WORD>(date.year());
    systemTime.wMonth = static_cast<WORD>(date.month());
    systemTime.wDayOfWeek = static_cast<WORD>(date.dayOfWeek() % 7);
    systemTime.wDay = static_cast<WORD>(date.day());
    systemTime.wHour = static_cast<WORD>(time.hour());
    systemTime.wMinute = static_cast<WORD>(time.minute());
    systemTime.wSecond = static_cast<WORD>(time.second());
    systemTime.wMilliseconds = static_cast<WORD>(time.msec());

    if(SetSystemTime(&systemTime))
    {
        ui->label_GPSSyncStatus->setText("sync_status: synchronized");
        DBGOPT_INFO("System UTC synchronized to GPS, previous difference = %lld ms", differenceMs);
    }
    else
    {
        const DWORD errorCode = GetLastError();
        gpsSyncEnabled = false;
        ui->pBtn_GPSSyncSystemTime->setText("Start system time sync");
        ui->label_GPSSyncStatus->setText("sync_status: failed, error " + QString::number(errorCode));
        DBGOPT_ERROR("SetSystemTime failed, Windows error = %lu", errorCode);
    }
#else
    gpsSyncEnabled = false;
    ui->pBtn_GPSSyncSystemTime->setText("Start system time sync");
    ui->label_GPSSyncStatus->setText("sync_status: Windows only");
#endif
}

void gpsTool::syncSystemTimeOnTimer()
{
    if(ix.fps < 1.0 || (gpsSyncEnabled && gpsFrameAge.isValid() && gpsFrameAge.elapsed() > 5000))
        syncSystemTimeToGPS();
}

void gpsTool::on_pBtnCloseTool_clicked()
{
    this->close();
}

//void gpsTool::on_pBtn_GPSMode_clicked()
//{
//    if(ix.CamID.contains("QHY990") || ix.CamID.contains("QHY991"))
//    {
//        OutputDebug("EZCAP | gpsTool.cpp | on_comBox_GPSOnOff_currentTextChanged | 0xbd");
//        uint16_t index = 156;
//        uint16_t value = ui->spinBox_GPSMode->value();
//        uint8_t xdata[10] = { 0 };
//        uint32_t ret = libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, value, index, 1, xdata);
//    }
//}


void gpsTool::on_spinBox_GPSAntennaMode_valueChanged(int arg1)
{
//    if(ix.CamID.contains("QHY990") || ix.CamID.contains("QHY991"))
//    {
        OutputDebug("EZCAP | gpsTool.cpp | on_comBox_GPSOnOff_currentTextChanged | 0xbd");
        uint16_t index = 156;
        uint16_t value = ui->spinBox_GPSAntennaMode->value();
        uint8_t xdata[10] = { 0 };
        uint32_t ret = libqhyccd->QHYCCDVendRequestWrite(camhandle, 0xbd, value, index, 1, xdata);
//    }
}

