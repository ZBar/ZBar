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

#include <qevent.h>
#include <qurl.h>
#include <qx11info_x11.h>
#include <zbar/QZBar.h>
#include "QZBarThread.h"

using namespace zbar;

QZBar::QZBar (QWidget *parent)
    : QWidget(parent),
      thread(NULL),
      _videoDevice(),
      _videoEnabled(false)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);

    QSizePolicy sizing(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizing.setHeightForWidth(true);
    setSizePolicy(sizing);

    thread = new QZBarThread;
    if(testAttribute(Qt::WA_WState_Created))
        thread->window.attach(x11Info().display(), winId());
    connect(thread, SIGNAL(videoOpened(bool)),
            this, SIGNAL(videoOpened(bool)));
    connect(this, SIGNAL(videoOpened(bool)),
            this, SLOT(sizeChange()));
    connect(thread, SIGNAL(update()),
            this, SLOT(update()));
    connect(thread, SIGNAL(decoded(int, const QString&)),
            this, SIGNAL(decoded(int, const QString&)));
    connect(thread, SIGNAL(decodedText(const QString&)),
            this, SIGNAL(decodedText(const QString&)));
    thread->start();
}

QZBar::~QZBar ()
{
    if(thread) {
        thread->window.attach(NULL);
        thread->pushEvent(new QEvent((QEvent::Type)QZBarThread::Exit));
        thread->wait();
        thread = NULL;
    }
}

QString QZBar::videoDevice () const
{
    return(_videoDevice);
}

void QZBar::setVideoDevice (const QString& videoDevice)
{
    if(!thread)
        return;
    if(_videoDevice != videoDevice) {
        _videoDevice = videoDevice;
        _videoEnabled = !_videoDevice.isEmpty();
        thread->pushEvent(new QZBarThread::VideoDeviceEvent(videoDevice));
    }
}

bool QZBar::isVideoEnabled () const
{
    return(_videoEnabled);
}

void QZBar::setVideoEnabled (bool videoEnabled)
{
    if(!thread)
        return;
    if(_videoEnabled != videoEnabled) {
        _videoEnabled = videoEnabled;
        thread->pushEvent(new QZBarThread::VideoEnabledEvent(videoEnabled));
    }
}

bool QZBar::isVideoOpened () const
{
    if(!thread)
        return(false);
    QMutexLocker locker(&thread->mutex);
    return(thread->_videoOpened);
}

void QZBar::scanImage (const QImage &image)
{
    if(!thread)
        return;
    thread->pushEvent(new QZBarThread::ScanImageEvent(image));
}

void QZBar::dragEnterEvent (QDragEnterEvent *event)
{
    if(event->mimeData()->hasImage() ||
       event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void QZBar::dropEvent (QDropEvent *event)
{
    if(event->mimeData()->hasImage()) {
        QImage image = qvariant_cast<QImage>(event->mimeData()->imageData());
        scanImage(image);
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
    else {
        // FIXME TBD load URIs and queue for processing
#if 0
        std::cerr << "drop: "
                  << event->mimeData()->formats().join(", ").toStdString()
                  << std::endl;
        QList<QUrl> urls = event->mimeData()->urls();
        for(int i = 0; i < urls.size(); ++i)
            std::cerr << "[" << i << "] "
                      << urls.at(i).toString().toStdString()
                      << std::endl;
#endif
    }
}

QSize QZBar::sizeHint () const
{
    if(!thread)
        return(QSize(640, 480));
    QMutexLocker locker(&thread->mutex);
    return(QSize(thread->reqWidth, thread->reqHeight));
}

int QZBar::heightForWidth (int width) const
{
    if(thread) {
        QMutexLocker locker(&thread->mutex);
        int base_width = thread->reqWidth;
        int base_height = thread->reqHeight;
        if(base_width > 0 && base_height > 0)
            return(base_height * width / base_width);
    }
    return(width * 3 / 4);
}

void QZBar::paintEvent (QPaintEvent *)
{
    if(thread)
        thread->window.redraw();
}

void QZBar::resizeEvent (QResizeEvent *event)
{
    QSize size = event->size();
    if(thread)
        thread->window.resize(size.rwidth(), size.rheight());
}

void QZBar::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::ParentChange)
        thread->window.attach(x11Info().display(), winId());
}

void QZBar::sizeChange ()
{
    update();
    updateGeometry();
}
