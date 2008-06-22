//------------------------------------------------------------------------
//  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the Zebra Barcode Library.
//
//  The Zebra Barcode Library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The Zebra Barcode Library is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the Zebra Barcode Library; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zebra
//------------------------------------------------------------------------

#include <iostream>
#include <QApplication>
#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QTextEdit>
#include <zebra/QZebra.h>

using namespace zebra;

extern "C" {
int scan_video(void *add_device,
               void *userdata,
               const char *default_device);
}

void add_device (QComboBox *list,
                 const char *device)
{
    list->addItem(QString(device));
}

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;                     // the top level window
    QVBoxLayout vbox;                   // w/contents arranged vertically
    QComboBox videoList;                // drop-down list of video devices
    QTextEdit results;                  // text box for results
    zebra::QZebra zebra;                // video barcode scanner

    window.setLayout(&vbox);
    vbox.addWidget(&videoList);
    vbox.addWidget(&zebra);
    vbox.addWidget(&results);

    results.setReadOnly(true);

    // directly connect combo box change signal to scanner video open
    QObject::connect(&videoList, SIGNAL(currentIndexChanged(const QString&)),
                     &zebra, SLOT(setVideoDevice(const QString&)));

    // directly connect video scanner decode result to display in text box
    QObject::connect(&zebra, SIGNAL(decodedText(const QString&)),
                     &results, SLOT(append(const QString&)));

    const char *dev = NULL;
    if(argc > 1)
        dev = argv[1];

    int active = scan_video((void*)add_device, &videoList, dev);
    if(active >= 0)
        videoList.setCurrentIndex(active);

    window.show();
    return(app.exec());
}
