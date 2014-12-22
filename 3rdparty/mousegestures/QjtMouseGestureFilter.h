/*
 * This file is part of the mouse gesture package.
 * Copyright (C) 2006 Johan Thelin <e8johan@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the
 * following conditions are met:
 *
 *   - Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer.
 *   - Redistributions in binary form must reproduce the
 *     above copyright notice, this list of conditions and
 *     the following disclaimer in the documentation and/or
 *     other materials provided with the distribution.
 *   - The names of its contributors may be used to endorse
 *     or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef QJTMOUSEGESTUREFILTER_H
#define QJTMOUSEGESTUREFILTER_H

#include <QObject>
#include <QEvent>
#include <QMouseEvent>

class QjtMouseGesture;

/*
 *  The QjtMouseGestureFilter class is a mouse
 *  gesture recognizing event filter.
 *
 *  Use it by creating it, install it as a filter
 *  for the affected widget and add QjtMouseGesture
 *  instances to it.
 *
 */
class QjtMouseGestureFilter : public QObject
{
public:
    /*
     *  The gestureButton controls which mouse button
     *  that has to be pressed during the mouse gesture.
     *  Notice that this all events for this button are
     *  swallowed by the filter.
     */
    QjtMouseGestureFilter(QObject *parent = 0 );
    ~QjtMouseGestureFilter();

    /*
     *  Adds a gesture to the filter.
     */
    void addGesture( QjtMouseGesture *gesture );
    void startGesture( QObject *obj, QMouseEvent *event );
    bool endGesture( QObject *obj, QMouseEvent *event );

    /*
     *  Clears the filter from gestures.
     *
     *  If deleteGestures is true, the QjtMouseGesture objects are deleted.
     */
    void clearGestures( bool deleteGestures = false );

protected:
    bool eventFilter( QObject *obj, QEvent *event );

private:
    bool mouseMoveEvent( QObject *obj, QMouseEvent *event );

    class Private;
    Private *d;
};

#endif // QJTMOUSEGESTUREFILTER_H
