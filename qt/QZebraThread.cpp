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
#include "QZebraThread.h"

using namespace zebra;

static const QString textFormat("%1%2:%3");

QZebraThread::QZebraThread ()
    : reqWidth(DEFAULT_WIDTH),
      reqHeight(DEFAULT_HEIGHT),
      video(NULL),
      image(NULL),
      running(true),
      videoRunning(false),
      videoEnabled(false)
{
    scanner.set_handler(*this);
}

void QZebraThread::image_callback (Image &image)
{
    for(Image::SymbolIterator sym = image.symbol_begin();
        sym != image.symbol_end();
        ++sym)
        if(!sym->get_count()) {
            QString data = QString::fromStdString(sym->get_data());
            emit decoded(sym->get_type(), data);

            emit decodedText(textFormat.arg(
                QString::fromStdString(sym->get_type_name()),
                QString::fromStdString(sym->get_addon_name()),
                data));
        }
}

void QZebraThread::processImage (Image &image)
{
    scanner.scan(image);
    window.draw(image);
    if(this->image && this->image != &image) {
        delete this->image;
        this->image = NULL;
    }
    emit update();
}

void QZebraThread::enableVideo (bool enable)
{
    if(!video) {
        videoRunning = videoEnabled = false;
        return;
    }
    try {
        scanner.enable_cache(enable);
        video->enable(enable);
        videoRunning = enable;
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    if(!enable) {
        // release video image and revert to logo
        clear();
        emit update();
    }
}

void QZebraThread::openVideo (const QString &device)
{
    if(videoRunning)
        enableVideo(false);

    {
        QMutexLocker locker(&mutex);
        videoEnabled = _videoOpened = false;
        reqWidth = DEFAULT_WIDTH;
        reqHeight = DEFAULT_HEIGHT;
    }

    // ensure old video doesn't have image ref
    // (FIXME handle video destroyed w/images outstanding)
    clear();
    emit update();

    if(video) {
        delete video;
        video = NULL;
        emit videoOpened(false);
    }

    if(device.isEmpty())
        return;

    try {
        std::string devstr = device.toStdString();
        video = new Video(devstr);
        negotiate_format(*video, window);
        {
            QMutexLocker locker(&mutex);
            videoEnabled = _videoOpened = true;
            reqWidth = video->get_width();
            reqHeight = video->get_height();
        }
        emit videoOpened(true);
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        emit videoOpened(false);
    }
}


void QZebraThread::videoDeviceEvent (VideoDeviceEvent *e)
{
    openVideo(e->device);
}

void QZebraThread::videoEnabledEvent (VideoEnabledEvent *e)
{
    if(videoRunning && !e->enabled)
        enableVideo(false);
    videoEnabled = e->enabled;
}

void QZebraThread::scanImageEvent (ScanImageEvent *e)
{
    if(videoRunning)
        enableVideo(false);

    try {
        image = new QZebraImage(e->image);
        processImage(*image);
    }
    catch(std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        clear();
    }
}

bool QZebraThread::event (QEvent *e)
{
    switch(e->type()) {
    case VideoDevice:
        videoDeviceEvent((VideoDeviceEvent*)e);
        break;
    case VideoEnabled:
        videoEnabledEvent((VideoEnabledEvent*)e);
        break;
    case ScanImage:
        scanImageEvent((ScanImageEvent*)e);
        break;
    case Exit:
        if(videoRunning)
            enableVideo(false);
        running = false;
        break;
    default:
        return(false);
    }
    return(true);
}


void QZebraThread::run ()
{
    QEvent *e = NULL;
    while(running) {
        if(!videoEnabled) {
            QMutexLocker locker(&mutex);
            while(queue.isEmpty())
                newEvent.wait(&mutex);
            e = queue.takeFirst();
        }
        else {
            // release reference to any previous QImage
            clear();
            enableVideo(true);

            while(videoRunning && !e) {
                try {
                    Image image = video->next_image();
                    processImage(image);
                }
                catch(std::exception &e) {
                    std::cerr << "ERROR: " << e.what() << std::endl;
                    enableVideo(false);
                    openVideo("");
                }
                QMutexLocker locker(&mutex);
                if(!queue.isEmpty())
                    e = queue.takeFirst();
            }

            if(videoRunning)
                enableVideo(false);
        }
        if(e) {
            event(e);
            delete e;
            e = NULL;
        }
    }
    clear();
    openVideo("");
}
