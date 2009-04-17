//------------------------------------------------------------------------
//  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------

#include <QApplication>
#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QImage>
#include <zbar/QZBar.h>

#define TEST_IMAGE_FORMATS \
    "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.ppm *.pgm *.pbm *.tiff *.xpm *.xbm)"

extern "C" {
int scan_video(void *add_device,
               void *userdata,
               const char *default_device);
}

class TestQZBar : public QWidget
{
    Q_OBJECT

protected:
    static void add_device (QComboBox *list,
                            const char *device)
    {
        list->addItem(QString(device));
    }

public:
    TestQZBar (const char *default_device)
    {
        // drop-down list of video devices
        QComboBox *videoList = new QComboBox;

        // toggle button to disable/enable video
        QPushButton *statusButton = new QPushButton;

        QStyle *style = QApplication::style();
        QIcon statusIcon = style->standardIcon(QStyle::SP_DialogNoButton);
        QIcon yesIcon = style->standardIcon(QStyle::SP_DialogYesButton);
        statusIcon.addPixmap(yesIcon.pixmap(QSize(128, 128),
                                            QIcon::Normal, QIcon::On),
                             QIcon::Normal, QIcon::On);

        statusButton->setIcon(statusIcon);
        statusButton->setText("&Enable");
        statusButton->setCheckable(true);
        statusButton->setEnabled(false);

        // command button to open image files for scanning
        QPushButton *openButton = new QPushButton("&Open");
        QIcon openIcon = style->standardIcon(QStyle::SP_DialogOpenButton);
        openButton->setIcon(openIcon);

        // collect video list and buttons horizontally
        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addWidget(videoList, 5);
        hbox->addWidget(statusButton, 1);
        hbox->addWidget(openButton, 1);

        // video barcode scanner
        zbar = new zbar::QZBar;
        zbar->setAcceptDrops(true);

        // text box for results
        QTextEdit *results = new QTextEdit;
        results->setReadOnly(true);

        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->addLayout(hbox);
        vbox->addWidget(zbar);
        vbox->addWidget(results);

        setLayout(vbox);

        videoList->addItem("");
        int active = scan_video((void*)add_device, videoList, default_device);

        // directly connect combo box change signal to scanner video open
        connect(videoList, SIGNAL(currentIndexChanged(const QString&)),
                zbar, SLOT(setVideoDevice(const QString&)));

        // directly connect status button state to video enabled state
        connect(statusButton, SIGNAL(toggled(bool)),
                zbar, SLOT(setVideoEnabled(bool)));

        // also update status button state when video is opened/closed
        connect(zbar, SIGNAL(videoOpened(bool)),
                statusButton, SLOT(setEnabled(bool)));

        // enable/disable status button when video is opened/closed
        connect(zbar, SIGNAL(videoOpened(bool)),
                statusButton, SLOT(setChecked(bool)));

        // prompt for image file to scan when openButton is clicked
        connect(openButton, SIGNAL(clicked()), SLOT(openImage()));

        // directly connect video scanner decode result to display in text box
        connect(zbar, SIGNAL(decodedText(const QString&)),
                results, SLOT(append(const QString&)));

        if(active >= 0)
            videoList->setCurrentIndex(active);
    }

public Q_SLOTS:
    void openImage ()
    {
        file = QFileDialog::getOpenFileName(this, "Open Image", file,
                                            TEST_IMAGE_FORMATS);
        if(!file.isEmpty())
            zbar->scanImage(QImage(file));
    }

private:
    QString file;
    zbar::QZBar *zbar;
};

#include "moc_test_qt.h"

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);

    const char *dev = NULL;
    if(argc > 1)
        dev = argv[1];

    TestQZBar window(dev);
    window.show();
    return(app.exec());
}
