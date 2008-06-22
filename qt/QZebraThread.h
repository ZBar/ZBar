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
#ifndef _QZEBRATHREAD_H_
#define _QZEBRATHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <zebra/QZebra.h>
#include <zebra.h>

namespace zebra {

class QZebraThread
    : public QThread,
      public Image::Handler
{
    Q_OBJECT

public:
    QMutex mutex;
    QWaitCondition cond;

    Video *video;
    Window window;
    ImageScanner scanner;

    QString videoDevice;
    bool videoEnabled;

    enum {
        IDLE,
        VIDEO_INIT,
        VIDEO_READY,
        IMAGE,
        EXIT = -1
    } state;

    QZebraThread();
    ~QZebraThread();

Q_SIGNALS:
    void videoOpened();
    void update();
    void decoded(int type, const QString& data);
    void decodedText(const QString& data);

protected:
    void run();

    Video *openVideo(std::string device);
    void processImage(Image &image);

    void image_callback(Image &image);

    QString textFormat;
};

};

#endif
