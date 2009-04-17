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
#ifndef _QZBARTHREAD_H_
#define _QZBARTHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QEvent>
#include <zbar/QZBarImage.h>
#include <zbar/QZBar.h>
#include <zbar.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

namespace zbar {

class QZBarThread
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

    QZBarThread();

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
    QZBarImage *image;
    bool running;
    bool videoRunning;
    bool videoEnabled;
};

};

#endif
