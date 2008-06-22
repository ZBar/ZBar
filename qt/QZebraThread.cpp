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

QZebraThread::QZebraThread ()
    : video(NULL),
      state(IDLE),
      textFormat("%1%2:%3")
{
    scanner.set_handler(*this);
}

QZebraThread::~QZebraThread ()
{
    if(video) {
        delete video;
        video = NULL;
    }
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


Video *QZebraThread::openVideo (std::string device)
{
    if(video) {
        // ensure old video doesn't have image ref
        // (FIXME handle video destroyed w/images outstanding)
        window.clear();
        delete video;
        video = NULL;
    }
    video = new Video(device);
    negotiate_format(*video, window);
    emit videoOpened();
    return(NULL);
}

void QZebraThread::processImage (Image &image)
{
    scanner.scan(image);
    window.draw(image);
    emit update();
}


void QZebraThread::run ()
{
    mutex.lock();
    while(state != EXIT) {
        while(state == IDLE ||
              state == VIDEO_READY && !videoEnabled)
            cond.wait(&mutex);

        if(state == VIDEO_INIT) {
            QString devstr = QString(videoDevice);
            std::string dev = devstr.toStdString();
            mutex.unlock();

            try {
                openVideo(dev);

                mutex.lock();
                if(state == VIDEO_INIT && devstr == videoDevice)
                    state = VIDEO_READY;
            }
            catch(std::exception &e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                mutex.lock();
                state = IDLE;
            }
        }

        bool enabled = false;
        if(state == VIDEO_READY && videoEnabled) {
            mutex.unlock();
            try {
                video->enable();
                scanner.enable_cache();
                enabled = true;
                mutex.lock();
            }
            catch(std::exception &e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                mutex.lock();
                state = IDLE;
            }
        }

        while(state == VIDEO_READY && videoEnabled) {
            mutex.unlock();
            try {
                Image image = video->next_image();
                processImage(image);
                mutex.lock();
            }
            catch(std::exception &e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                mutex.lock();
                state = IDLE;
            }
            
        }

        if(enabled) {
            mutex.unlock();
            try {
                scanner.enable_cache(false);
                video->enable(false);
            }
            catch(std::exception &e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
            }
            mutex.lock();
        }
    }
    mutex.unlock();
}
