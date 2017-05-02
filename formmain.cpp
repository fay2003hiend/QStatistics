#include "formmain.h"
#include "ui_formmain.h"
#include <QFileDialog>
#include <QDateTime>
#include <QMap>
#include <QSettings>
#include <QMimeData>

#include <math.h>

StatisticData::StatisticData()
{

}

double StatisticData::getMinValue()
{
    double ret = data[0];
    for(int i=1; i<data.size(); i++)
    {
        if (data[i] < ret)
            ret = data[i];
    }
    return ret;
}

double StatisticData::getMaxValue()
{
    double ret = data[0];
    for(int i=1; i<data.size(); i++)
    {
        if (data[i] > ret)
            ret = data[i];
    }
    return ret;
}

double StatisticData::getMedianValue()
{
    double ret;
    QList<double> tmp = data;

    qSort(tmp.begin(), tmp.end());
    if (tmp.size() % 2 == 0)
    {
        ret = (tmp[tmp.size()/2] + tmp[tmp.size()/2-1])/2;
    }
    else
    {
        ret = tmp[tmp.size()/2];
    }
    return ret;
}

double StatisticData::getMeanValue()
{
    double sum = 0;
    foreach(double d, data)
    {
        sum += d;
    }
    return sum / data.size();
}

double StatisticData::getStdDeviation()
{
    double ret = 0.0f;
    double mean = getMeanValue();
    foreach (double d, data)
    {
        ret += pow(d - mean, 2);
    }
    return sqrt(ret / data.size());
}

#define SET_VAL(x) conf.setValue(#x, x)
#define GET_VAL(x) conf.value(#x)

FormMain::FormMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormMain),
    _skip_percentage(0)
{
    ui->setupUi(this);

    connect(ui->slide_skip_percentage, SIGNAL(valueChanged(int)), SLOT(slot_changeSkipPercentage(int)));
    connect(ui->btn_reload, SIGNAL(clicked(bool)), SLOT(slot_reload()));
    connect(ui->btn_open, SIGNAL(clicked(bool)), SLOT(slot_open()));

    QSettings conf("Qst.ini", QSettings::IniFormat);
    _path = GET_VAL(_path).toString();
    _separator = GET_VAL(_separator).toString();
    _columns_to_show = GET_VAL(_columns_to_show).toString();

    _skip_percentage = GET_VAL(_skip_percentage).toInt();

    // apply to UI
    ui->edit_separator->setText(_separator);
    ui->edit_columns_to_show->setText(_columns_to_show);

    ui->slide_skip_percentage->setValue(_skip_percentage);
    slot_changeSkipPercentage(_skip_percentage);

    this->setAcceptDrops(true);
}

FormMain::~FormMain()
{
    QSettings conf("Qst.ini", QSettings::IniFormat);
    SET_VAL(_path);
    SET_VAL(_separator);
    SET_VAL(_columns_to_show);

    SET_VAL(_skip_percentage);
    conf.sync();

    delete ui;
}

void FormMain::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void FormMain::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    foreach (const QUrl &url, urls)
    {
        if (url.isLocalFile())
        {
            _path = url.toLocalFile();
            slot_reload();
        }
    }
}

void FormMain::slot_changeSkipPercentage(int value)
{
    ui->label_skip_percentage->setText(tr("Skip first %1% samples").arg(value));
    _skip_percentage = value;
}

void FormMain::slot_open()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open data file"), _path);
    if (path.isEmpty())
    {
        return;
    }

    _path = path;
    slot_reload();
}

QString get_columns(const QList<int>& list)
{
    QString ret = "";
    foreach(int val, list)
    {
        ret.append(QString::number(val));
        ret.append(' ');
    }
    return ret;
}

void FormMain::slot_reload()
{
    if (_path.isEmpty())
    {
        return;
    }

    // apply separator and columns
    _separator = ui->edit_separator->text();
    if (_separator.isEmpty())
    {
        _separator = ",";
    }

    QList<int> columnsList;
    QStringList columns = ui->edit_columns_to_show->text().split(',', QString::SkipEmptyParts);
    if (columns.count() < 1)
    {
        _columns_to_show = "0";
        columnsList.append(0);
    }
    else
    {
        foreach(QString col, columns)
        {
            columnsList.append(col.toInt());
        }
        _columns_to_show = columns.join(",");
    }

    // show settings
    QDateTime now = QDateTime::currentDateTime();
    ui->textBrowser->append(tr("<br><br>%1").arg(now.toString("dd-MMM-yyyy HH:mm:ss")));
    ui->textBrowser->append(tr("Loading file <font color=blue>%1</font>").arg(_path));
    ui->textBrowser->append(tr("Separator: <font color=red>%1</font> SkipPercentage: <font color=red>%2%</font> "
                               "ColumnsToShow: <font color=red>%4</font>")
                            .arg(_separator)
                            .arg(_skip_percentage)
                            .arg(_columns_to_show));

    // read file
    QFile f(_path);
    if (!f.open(QIODevice::ReadOnly))
    {
        ui->textBrowser->append(tr("Open file failed!"));
        return;
    }

    QMap<int, StatisticData *> dataset;
    foreach(int column, columnsList)
    {
        dataset.insert(column, new StatisticData());
    }

    char buf[1024];
    qint64 n;

    while((n = f.readLine(buf, sizeof(buf))) >= 0)
    {
        while (n > 0 && (buf[n-1] == '\r' || buf[n-1] == '\n'))
        {
            --n;
        }
        if (n == 0)
        {
            continue;
        }

        QString line = QString::fromLocal8Bit(buf, n);

        QStringList l = line.split(_separator, QString::SkipEmptyParts);

        foreach(int column, columnsList)
        {
            if (column < l.size())
            {
                StatisticData *p = dataset.value(column);
                bool ok = false;
                double val = l[column].toDouble(&ok);
                if (ok)
                {
                    p->data.append(val);
                }
            }
        }
    }

    foreach(int column, columnsList)
    {
        StatisticData *p = dataset.value(column);

        ui->textBrowser->append(tr("<font color=blue>Found %1 rows of data for column %2</font>")
                                .arg(p->data.size())
                                .arg(column));

        if (_skip_percentage > 0)
        {
            int rows_to_remove = (p->data.size() * _skip_percentage) / 100;
            ui->textBrowser->append(tr("<font color=orange>Skip %1 rows</font>").arg(rows_to_remove));
            while(rows_to_remove--)
            {
                p->data.removeFirst();
            }
        }

        if (p->data.size() < 1)
        {
            continue;
        }

        ui->textBrowser->append(tr("min\tmax\tavg\tmedian\tstdev"));
        ui->textBrowser->append(tr("%1\t%2\t%3\t%4\t%5")
                                .arg(p->getMinValue())
                                .arg(p->getMaxValue())
                                .arg(p->getMeanValue())
                                .arg(p->getMedianValue())
                                .arg(p->getStdDeviation()));

        delete p;
    }

    f.close();
}
