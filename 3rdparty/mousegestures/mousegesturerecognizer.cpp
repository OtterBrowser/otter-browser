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

#include "mousegesturerecognizer.h"

using namespace Gesture;

// Private data structure

struct Pos
{
    Pos( int ix, int iy ) : x(ix), y(iy) {}

    int x, y;
};

typedef std::list<Pos> PosList;
typedef std::list<GestureDefinition> GestureList;

struct MouseGestureRecognizer::Private
{
    PosList positions;
    GestureList gestures;

    int minimumMovement2;
    double minimumMatch;
};

// Support functions

PosList limitDirections( const PosList &positions );
PosList simplify( const PosList &positions );
PosList removeShortest( const PosList &positions );
int calcLength( const PosList &positions );

// Class implementation

MouseGestureRecognizer::MouseGestureRecognizer( int minimumMovement, double minimumMatch )
{
    d = new Private;
    d->minimumMovement2 = minimumMovement*minimumMovement;
    d->minimumMatch = minimumMatch;
}

MouseGestureRecognizer::~MouseGestureRecognizer()
{
    delete d;
}

void MouseGestureRecognizer::addGestureDefinition( const GestureDefinition &gesture )
{
    d->gestures.push_back( gesture );
}

void MouseGestureRecognizer::clearGestureDefinitions()
{
    d->gestures.clear();
}

void MouseGestureRecognizer::startGesture( int x, int y )
{
    d->positions.clear();
    d->positions.push_back( Pos( x, y ) );
}

bool MouseGestureRecognizer::endGesture( int x, int y )
{
    if( x != d->positions.back().x || y != d->positions.back().y )
        d->positions.push_back( Pos( x, y ) );

    bool matched = false;

    if( d->positions.size() > 1 )
        matched = recognizeGesture();

    d->positions.clear();

    return matched;
}

void MouseGestureRecognizer::abortGesture()
{
    d->positions.clear();
}

void MouseGestureRecognizer::addPoint( int x, int y )
{
    int dx, dy;

    dx = x-d->positions.back().x;
    dy = y-d->positions.back().y;

    if( dx*dx+dy*dy >= d->minimumMovement2 )
    {
        d->positions.push_back( Pos( x, y ) );
    }
}

bool MouseGestureRecognizer::recognizeGesture()
{
    PosList directions = simplify( limitDirections( d->positions ) );
    double minLength = calcLength( directions ) * d->minimumMatch;

    while( directions.size() > 0 && calcLength(directions) > minLength )
    {
        for( GestureList::const_iterator gi = d->gestures.begin(); gi != d->gestures.end(); ++gi )
        {
            if( gi->directions.size() == directions.size() )
            {
                bool match = true;
                PosList::const_iterator pi = directions.begin();
                for( DirectionList::const_iterator di = gi->directions.begin(); di != gi->directions.end() && match; ++di, ++pi )
                {
                    switch( *di )
                    {
                    case Up:
                        if( pi->y >= 0 )
                            match = false;

                        break;
                    case Down:
                        if( pi->y <= 0 )
                            match = false;

                        break;
                    case Left:
                        if( pi->x >= 0 )
                            match = false;

                        break;
                    case Right:
                        if( pi->x <= 0 )
                            match = false;

                        break;
                    case AnyHorizontal:
                        if( pi->x == 0 )
                            match = false;

                        break;
                    case AnyVertical:
                        if( pi->y == 0 )
                            match = false;

                        break;
                    case NoMatch:
                        match = false;

                        break;
                    }
                }

                if( match )
                {
                    gi->callbackClass->callback();
                    return true;
                }
            }
        }

        directions = simplify( removeShortest( directions ) );
    }

    for( GestureList::const_iterator gi = d->gestures.begin(); gi != d->gestures.end(); ++gi )
    {
        if( gi->directions.size() == 1 )
        {
            if( gi->directions.back() == NoMatch )
            {
                gi->callbackClass->callback();
                return true;
            }
        }
    }

    return false;
}

// Support functions implementation

/*
 *  limitDirections - limits the directions of a list to up, down, left or right.
 *
 *  Notice! This function converts the list to a set of relative moves instead of a set of screen coordinates.
 */
PosList limitDirections( const PosList &positions )
{
    PosList res;
    int lastx, lasty;
    bool firstTime = true;

    for( PosList::const_iterator ii = positions.begin(); ii != positions.end(); ++ii )
    {
        if( firstTime )
        {
            lastx = ii->x;
            lasty = ii->y;

            firstTime = false;
        }
        else
        {
            int dx, dy;

            dx = ii->x - lastx;
            dy = ii->y - lasty;

            if( dy > 0 )
            {
                if( dx > dy || -dx > dy )
                    dy = 0;
                else
                    dx = 0;
            }
            else
            {
                if( dx > -dy || -dx > -dy )
                    dy = 0;
                else
                    dx = 0;
            }

            res.push_back( Pos( dx, dy ) );

            lastx = ii->x;
            lasty = ii->y;
        }
    }

    return res;
}

/*
 *  simplify - joins together contignous movements in the same direction.
 *
 *  Notice! This function expects a list of limited directions.
 */
PosList simplify( const PosList &positions )
{
    PosList res;
    int lastdx = 0, lastdy = 0;
    bool firstTime = true;

    for( PosList::const_iterator ii = positions.begin(); ii != positions.end(); ++ii )
    {
        if( firstTime )
        {
            lastdx = ii->x;
            lastdy = ii->y;

            firstTime = false;
        }
        else
        {
            bool joined = false;

            if( (lastdx > 0 && ii->x > 0) || (lastdx < 0 && ii->x < 0) )
            {
                lastdx += ii->x;
                joined = true;
            }

            if( (lastdy > 0 && ii->y > 0) || (lastdy < 0 && ii->y < 0) )
            {
                lastdy += ii->y;
                joined = true;
            }

            if( !joined )
            {
                res.push_back( Pos( lastdx, lastdy ) );

                lastdx = ii->x;
                lastdy = ii->y;
            }
        }
    }

    if( lastdx != 0 || lastdy != 0 )
    {
        res.push_back( Pos( lastdx, lastdy ) );
    }

    return res;
}

/*
 *  removeShortest - removes the shortest segment from a list of movements.
 *
 *  If there are several equally short segments, the first one is removed.
 */
PosList removeShortest( const PosList &positions )
{
    PosList res;

    int shortestSoFar;
    PosList::const_iterator shortest;
    bool firstTime = true;

    for( PosList::const_iterator ii = positions.begin(); ii != positions.end(); ++ii )
    {
        if( firstTime )
        {
            shortestSoFar = ii->x*ii->x + ii->y*ii->y;
            shortest = ii;

            firstTime = false;
        }
        else
        {
            if( (ii->x*ii->x + ii->y*ii->y) < shortestSoFar )
            {
                shortestSoFar = ii->x*ii->x + ii->y*ii->y;
                shortest = ii;
            }
        }
    }

    for( PosList::const_iterator ii = positions.begin(); ii != positions.end(); ++ii )
    {
        if( ii != shortest )
            res.push_back( *ii );
    }

    return res;
}

/*
 *  calcLength - calculates the total length of the movements from a list of relative movements.
 */
int calcLength( const PosList &positions )
{
    int res = 0;

    for( PosList::const_iterator ii = positions.begin(); ii != positions.end(); ++ii )
    {
        if( ii->x > 0 )
            res += ii->x;
        else if( ii->x < 0 )
            res -= ii->x;
        else if( ii->y > 0 )
            res += ii->y;
        else
            res -= ii->y;
    }

    return res;
}
