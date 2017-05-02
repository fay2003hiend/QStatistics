#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QWidget>
#include <QList>
#include <QDragEnterEvent>
#include <QDropEvent>

namespace Ui {
class FormMain;
}

class StatisticData
{
public:
    QList<double> data;

    StatisticData();

    double getMinValue();
    double getMaxValue();

    double getMedianValue();
    double getMeanValue();
    double getStdDeviation();
};

class FormMain : public QWidget
{
    Q_OBJECT
    QString _path;
    QString _separator;
    QString _columns_to_show;

    int _skip_percentage;

public:
    explicit FormMain(QWidget *parent = 0);
    ~FormMain();

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private slots:
    void slot_changeSkipPercentage(int value);

    void slot_open();
    void slot_reload();

private:
    Ui::FormMain *ui;
};

#endif // FORMMAIN_H
