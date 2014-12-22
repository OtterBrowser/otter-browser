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

#ifndef MOUSEGESTURERECOGNIZER_H
#define MOUSEGESTURERECOGNIZER_H

#include <list>

namespace Gesture {

/*
 *  Sub-class and implement callback method and register along a gesture description.
 */
class MouseGestureCallback
{
public:
    virtual void callback() = 0;
};

/*
 *  The available directions.
 */
typedef enum { Up, Down, Left, Right, AnyHorizontal, AnyVertical, NoMatch } Direction;
/*
 *  A list of directions.
 */
typedef std::list<Direction> DirectionList;

/*
 *  Create lists of directions and connect to a callback.
 */
struct GestureDefinition
{
    GestureDefinition( const DirectionList &d, MouseGestureCallback *c ) : directions( d ), callbackClass( c ) {}

    DirectionList directions;
    MouseGestureCallback *callbackClass;
};

class MouseGestureRecognizer
{
public:
    MouseGestureRecognizer( int minimumMovement = 5, double minimumMatch = 0.9 );
    ~MouseGestureRecognizer();

    void addGestureDefinition( const GestureDefinition &gesture );
    void clearGestureDefinitions();

    void startGesture( int x, int y );
    void addPoint( int x, int y );
    bool endGesture( int x, int y );
    void abortGesture();

private:
    bool recognizeGesture();

    struct Private;
    Private *d;
};

};

#endif // MOUSEGESTURERECOGNIZER_H
