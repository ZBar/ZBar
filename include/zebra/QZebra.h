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
#ifndef _QZEBRA_H_
#define _QZEBRA_H_

#include <QWidget>

namespace zebra {

class QZebraThread;

class QZebra : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString videoDevice
               READ videoDevice
               WRITE setVideoDevice
               DESIGNABLE false)

    Q_PROPERTY(bool videoEnabled
               READ isVideoEnabled
               WRITE setVideoEnabled
               DESIGNABLE false)

public:

    QZebra(QWidget *parent = NULL);

    ~QZebra();

    QString videoDevice() const;
    bool isVideoEnabled() const;

    QSize sizeHint() const;
    int heightForWidth(int) const;

public Q_SLOTS:
    void setVideoDevice(const QString& videoDevice);
    void setVideoEnabled(bool videoEnabled = true);

Q_SIGNALS:
    void decoded(int type, const QString& data);
    void decodedText(const QString& text);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void changeEvent(QEvent*);

protected Q_SLOTS:
    void videoOpened();

private:
    QZebraThread *thread;
};

};

#endif
