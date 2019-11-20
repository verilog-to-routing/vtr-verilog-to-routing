#ifndef CLOCKCONFIG_H
#define CLOCKCONFIG_H

#include <QDialog>
#include "logicunit.h"

namespace Ui {
    class ClockConfig;
}

class ClockConfig : public QDialog
{
    Q_OBJECT

public:
    explicit ClockConfig(QWidget *parent = 0);
    ~ClockConfig();
    void passClockList(QList<LogicUnit*> clocklist);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::ClockConfig *ui;
    QList<LogicUnit*> clocks;
};

#endif // CLOCKCONFIG_H
