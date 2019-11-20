#include "clockconfig.h"
#include "ui_clockconfig.h"

ClockConfig::ClockConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClockConfig)
{
    ui->setupUi(this);
}

ClockConfig::~ClockConfig()
{
    delete ui;
}

void ClockConfig::on_buttonBox_accepted()
{
    //set the ratios in Odin. :)
    int i = 0;
    for(i = 0;i<clocks.length();i++){
        QString ratText = ui->tableWidget->item(i,1)->text();
        int rat = ratText.toInt();
        set_clock_ratio(rat,
                        clocks.at(i)->getOdinRef());
    }

}

void ClockConfig::on_buttonBox_rejected()
{
    // do nothing for now.
}

void ClockConfig::passClockList(QList<LogicUnit *> clocklist)
{
    clocks = clocklist;
    int i = 0;

    ui->tableWidget->setRowCount(clocklist.length());
    for(i = 0;i<clocklist.length();i++){
        int rat = get_clock_ratio(clocks.at(i)->getOdinRef());
        QString ratString;
        ratString.setNum(rat);
        QTableWidgetItem *clockName = new QTableWidgetItem(clocks.at(i)->getName());
        QTableWidgetItem *clockRatio = new QTableWidgetItem(ratString);

        ui->tableWidget->setItem(i,0,clockName);
        ui->tableWidget->setItem(i,1,clockRatio);
    }
}
