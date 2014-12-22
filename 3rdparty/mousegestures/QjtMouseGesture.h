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

#ifndef QJTMOUSEGESTURE_H
#define QJTMOUSEGESTURE_H

#include <QObject>
#include <QList>

#include "mousegesturerecognizer.h"

class GestureCallbackToSignal;

/*
 *  A Direction can be any of the following:
 *
 *    Up
 *    Down
 *    Left
 *    Right
 *    AnyHorizontal (Left or Right)
 *    AnyVertical (Up or Down)
 *
 *  In addition to these, the NoMatch enum is
 *  available. A gesture holding only the NoMatch
 *  enum is gestured when no other gesture can be
 *  matched to the mouse gesture performed.
 */
typedef Gesture::Direction Direction;

using Gesture::Up;
using Gesture::Down;
using Gesture::Left;
using Gesture::Right;
using Gesture::AnyHorizontal;
using Gesture::AnyVertical;
using Gesture::NoMatch;

/*
 *  A list of Directions
 */
typedef QList<Direction> DirectionList;

/*
 *  A mouse gesture is a list of Directions that
 *  trigger the gestured signal. Create instances
 *  and add to a QjtMouseGestureFilter object.
 */
class QjtMouseGesture : public QObject
{
    Q_OBJECT
public:
    QjtMouseGesture( const DirectionList &directions, QObject *parent = 0 );
    ~QjtMouseGesture();

    const DirectionList directions() const;

signals:
    void gestured();

private:
    friend class GestureCallbackToSignal;

    /*
     *  Emits the gestured signal.
     *
     *  Required to connect this to the toolkit
     *  independent recognizer in a tidy way.
     */
    void emitGestured();

    DirectionList m_directions;
};

#endif // QJTMOUSEGESTURE_H
