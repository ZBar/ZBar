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
#include <QEvent>
#include <zebra/QZebraImage.h>
#include <zebra/QZebra.h>
#include <zebra.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

namespace zebra {

class QZebraThread
    : public QThread,
      public Image::Handler
{
    Q_OBJECT

public:
    enum EventType {
        VideoDevice = QEvent::User,
        VideoEnabled,
        ScanImage,
        Exit = QEvent::MaxUser
    };

    class VideoDeviceEvent : public QEvent {
    public:
        VideoDeviceEvent (const QString &device)
            : QEvent((QEvent::Type)VideoDevice),
              device(device)
        { }
        const QString device;
    };

    class VideoEnabledEvent : public QEvent {
    public:
        VideoEnabledEvent (bool enabled)
            : QEvent((QEvent::Type)VideoEnabled),
              enabled(enabled)
        { }
        bool enabled;
    };

    class ScanImageEvent : public QEvent {
    public:
        ScanImageEvent (const QImage &image)
            : QEvent((QEvent::Type)ScanImage),
              image(image)
        { }
        const QImage image;
    };

    QMutex mutex;
    QWaitCondition newEvent;

    // message queue for events passed from main gui thread to processor.
    // (NB could(/should?) be QAbstractEventDispatcher except it doesn't
    //  work as documented!? ):
    // protected by mutex
    QList<QEvent*> queue;

    // shared state:
    // written by processor thread just after opening video or
    // scanning an image, read by main gui thread during size_request.
    // protected by mutex

    bool _videoOpened;
    unsigned reqWidth, reqHeight;

    // window is also shared: owned by main gui thread.
    // processor thread only calls draw(), clear() and negotiate_format().
    // protected by its own internal lock

    Window window;

    QZebraThread();

    void pushEvent (QEvent *e)
    {
        QMutexLocker locker(&mutex);
        queue.append(e);
        newEvent.wakeOne();
    }

Q_SIGNALS:
    void videoOpened(bool opened);
    void update();
    void decoded(int type, const QString &data);
    void decodedText(const QString &data);

protected:
    void run();

    void openVideo(const QString &device);
    void enableVideo(bool enable);
    void processImage(Image &image);

    void clear ()
    {
        window.clear();
        if(image) {
            delete image;
            image = NULL;
        }
    }

    virtual void image_callback(Image &image);

    virtual bool event(QEvent *e);
    virtual void videoDeviceEvent(VideoDeviceEvent *event);
    virtual void videoEnabledEvent(VideoEnabledEvent *event);
    virtual void scanImageEvent(ScanImageEvent *event);

private:
    Video *video;
    ImageScanner scanner;
    QZebraImage *image;
    bool running;
    bool videoRunning;
    bool videoEnabled;
};

};

#endif
