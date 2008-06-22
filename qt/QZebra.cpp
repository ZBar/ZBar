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

#include <QResizeEvent>
#include <QX11Info>
#include <zebra/QZebra.h>
#include "QZebraThread.h"

using namespace zebra;

QZebra::QZebra (QWidget *parent)
    : QWidget(parent),
      thread(new QZebraThread)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);

    QSizePolicy sizing(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizing.setHeightForWidth(true);
    setSizePolicy(sizing);

    if(testAttribute(Qt::WA_WState_Created))
        thread->window.attach(x11Info().display(), winId());
    connect(thread, SIGNAL(videoOpened()), this, SLOT(videoOpened()));
    connect(thread, SIGNAL(update()), this, SLOT(update()));
    connect(thread, SIGNAL(decoded(int, const QString&)),
            this, SIGNAL(decoded(int, const QString&)));
    connect(thread, SIGNAL(decodedText(const QString&)),
            this, SIGNAL(decodedText(const QString&)));
    thread->start();
}

QZebra::~QZebra ()
{
    if(thread) {
        thread->window.attach(NULL);
        thread->mutex.lock();
        thread->state = QZebraThread::EXIT;
        thread->cond.wakeOne();
        thread->mutex.unlock();
        thread = NULL;
    }
}

void QZebra::paintEvent (QPaintEvent *)
{
    if(thread)
        thread->window.redraw();
}

void QZebra::resizeEvent (QResizeEvent *event)
{
    QSize size = event->size();
    if(thread)
        thread->window.resize(size.rwidth(), size.rheight());
}

void QZebra::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::ParentChange)
        thread->window.attach(x11Info().display(), winId());
}

QSize QZebra::sizeHint () const
{
    if(thread) {
        thread->mutex.lock();
        if(thread->video) {
            QSize size(thread->video->get_width(),
                       thread->video->get_height());
            thread->mutex.unlock();
            return(size);
        }
        thread->mutex.unlock();
    }
    return(QSize(640, 480));
}

int QZebra::heightForWidth (int width) const
{
    if(thread) {
        thread->mutex.lock();
        if(thread->video) {
            int base_width = thread->video->get_width();
            int base_height = thread->video->get_height();
            thread->mutex.unlock();
            if(base_width > 0 && base_height > 0)
                return(base_height * width / base_width);
            return(width * 3 / 4);
        }
        thread->mutex.unlock();
    }
    return(width * 3 / 4);
}


void QZebra::videoOpened ()
{
    update();
    updateGeometry();
}


QString QZebra::videoDevice () const
{
    if(!thread)
        return("");

    thread->mutex.lock();
    QString result = thread->videoDevice;
    thread->mutex.unlock();
    return(result);
}

void QZebra::setVideoDevice (const QString& videoDevice)
{
    if(!thread)
        return;

    thread->mutex.lock();
    thread->videoEnabled = true;
    thread->videoDevice = videoDevice;
    thread->state = QZebraThread::VIDEO_INIT;
    thread->cond.wakeOne();
    thread->mutex.unlock();
}


bool QZebra::isVideoEnabled () const
{
    if(!thread)
        return(false);
    thread->mutex.lock();
    bool result = thread->videoEnabled;
    thread->mutex.unlock();
    return(result);
}

void QZebra::setVideoEnabled (bool videoEnabled)
{
    if(!thread)
        return;
    assert(testAttribute(Qt::WA_WState_Created));
    thread->mutex.lock();
    if(thread->videoEnabled != videoEnabled) {
        thread->videoEnabled = videoEnabled;
        thread->cond.wakeOne();
    }
    thread->mutex.unlock();
}
