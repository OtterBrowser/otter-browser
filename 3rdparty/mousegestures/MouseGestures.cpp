/*
 * This file is part of the mouse gesture package.
 * Copyright (C) 2006 Johan Thelin <e8johan@gmail.com>
 * Copyright (C) 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MouseGestures.h"

#include <vector>

namespace MouseGestures
{

// Private data structure

struct Position
{
	Position(int ix, int iy) : x(ix), y(iy) {}
	int x;
	int y;
};

typedef std::list<Position> PositionList;
typedef std::vector<ActionList> GestureList;

struct Recognizer::Private
{
	PositionList positions;
	GestureList gestures;
	int minimumMovement2;
	double minimumMatch;
};

// Support functions

PositionList limitDirections(const PositionList &positions);
PositionList simplify(const PositionList &positions);
PositionList removeShortest(const PositionList &positions);
int calculateLength(const PositionList &positions);

// Class implementation

Recognizer::Recognizer(int minimumMovement, double minimumMatch)
{
	d = new Private();
	d->gestures.reserve(25);
	d->minimumMovement2 = (minimumMovement * minimumMovement);
	d->minimumMatch = minimumMatch;
}

Recognizer::Recognizer(const Recognizer &other)
{
	d = new Private();
	d->gestures = other.d->gestures;
	d->minimumMovement2 = other.d->minimumMovement2;
	d->minimumMatch = other.d->minimumMatch;
}

Recognizer::~Recognizer()
{
	delete d;
}

void Recognizer::startGesture(int x, int y)
{
	d->positions.push_back(Position(x, y));
}

void Recognizer::addPosition(int x, int y)
{
	if (d->positions.empty())
	{
		d->positions.push_back(Position(x, y));

		return;
	}

	const int dx = (x - d->positions.back().x);
	const int dy = (y - d->positions.back().y);

	if (((dx * dx) + (dy * dy)) >= d->minimumMovement2)
	{
		d->positions.push_back(Position(x, y));
	}
}

int Recognizer::registerGesture(const ActionList &actions)
{
	d->gestures.push_back(actions);

	return (static_cast<int>(d->gestures.size()) - 1);
}

int Recognizer::endGesture()
{
	int gesture = -1;

	if (d->positions.size() > 1)
	{
		gesture = recognizeGesture();
	}

	d->positions.clear();

	return gesture;
}

int Recognizer::recognizeGesture()
{
	PositionList directions = simplify(limitDirections(d->positions));
	const double minimumLength = (calculateLength(directions) * d->minimumMatch);

	while (!directions.empty() && calculateLength(directions) > minimumLength)
	{
		for (unsigned int i = 0; i < d->gestures.size(); ++i)
		{
			if (d->gestures[i].size() == directions.size())
			{
				bool match = true;
				PositionList::const_iterator positionsIterator = directions.begin();

				for (ActionList::const_iterator directionsIterator = d->gestures[i].begin(); directionsIterator != d->gestures[i].end() && match; ++directionsIterator, ++positionsIterator)
				{
					switch (*directionsIterator)
					{
						case MoveUpMouseAction:
							if (positionsIterator->y >= 0)
							{
								match = false;
							}

							break;
						case MoveDownMouseAction:
							if (positionsIterator->y <= 0)
							{
								match = false;
							}

							break;
						case MoveLeftMouseAction:
							if (positionsIterator->x >= 0)
							{
								match = false;
							}

							break;
						case MoveRightMouseAction:
							if (positionsIterator->x <= 0)
							{
								match = false;
							}

							break;
						case MoveHorizontallyMouseAction:
							if (positionsIterator->x == 0)
							{
								match = false;
							}

							break;
						case MoveVerticallyMouseAction:
							if (positionsIterator->y == 0)
							{
								match = false;
							}

							break;
						default:
							match = false;

							break;
					}
				}

				if (match)
				{
					return static_cast<int>(i);
				}
			}
		}

		directions = simplify(removeShortest(directions));
	}

	return -1;
}

/*
 * limitDirections - limits the directions of a list to up, down, left or right.
 *
 * Notice! This function converts the list to a set of relative moves instead of a set of screen coordinates.
 */

PositionList limitDirections(const PositionList &positions)
{
	PositionList result;
	int lastX = 0;
	int lastY = 0;
	bool firstTime = true;

	for (PositionList::const_iterator positionsIterator = positions.begin(); positionsIterator != positions.end(); ++positionsIterator)
	{
		if (firstTime)
		{
			lastX = positionsIterator->x;
			lastY = positionsIterator->y;
			firstTime = false;
		}
		else
		{
			int dx = (positionsIterator->x - lastX);
			int dy = (positionsIterator->y - lastY);

			if (dy > 0)
			{
				if ((dx > dy) || (-dx > dy))
				{
					dy = 0;
				}
				else
				{
					dx = 0;
				}
			}
			else
			{
				if ((dx > -dy) || (-dx > -dy))
				{
					dy = 0;
				}
				else
				{
					dx = 0;
				}
			}

			result.push_back(Position(dx, dy));

			lastX = positionsIterator->x;
			lastY = positionsIterator->y;
		}
	}

	return result;
}

/*
 * simplify - joins together contignous movements in the same direction.
 *
 * Notice! This function expects a list of limited directions.
 */

PositionList simplify(const PositionList &positions)
{
	PositionList result;
	int lastDx = 0;
	int lastDy = 0;
	bool firstTime = true;

	for (PositionList::const_iterator positionsIterator = positions.begin(); positionsIterator != positions.end(); ++positionsIterator)
	{
		if (firstTime)
		{
			lastDx = positionsIterator->x;
			lastDy = positionsIterator->y;
			firstTime = false;
		}
		else
		{
			bool joined = false;

			if ((lastDx > 0 && positionsIterator->x > 0) || (lastDx < 0 && positionsIterator->x < 0))
			{
				lastDx += positionsIterator->x;
				joined = true;
			}

			if ((lastDy > 0 && positionsIterator->y > 0) || (lastDy < 0 && positionsIterator->y < 0))
			{
				lastDy += positionsIterator->y;
				joined = true;
			}

			if (!joined)
			{
				result.push_back(Position(lastDx, lastDy));

				lastDx = positionsIterator->x;
				lastDy = positionsIterator->y;
			}
		}
	}

	if (lastDx != 0 || lastDy != 0)
	{
		result.push_back(Position(lastDx, lastDy));
	}

	return result;
}

/*
 * removeShortest - removes the shortest segment from a list of movements.
 *
 * If there are several equally short segments, the first one is removed.
 */

PositionList removeShortest(const PositionList &positions)
{
	PositionList result;
	PositionList::const_iterator shortest;
	int shortestSoFar = 0;
	bool firstTime = true;

	for (PositionList::const_iterator positionsIterator = positions.begin(); positionsIterator != positions.end(); ++positionsIterator)
	{
		if (firstTime)
		{
			shortestSoFar = ((positionsIterator->x * positionsIterator->x) + (positionsIterator->y * positionsIterator->y));
			shortest = positionsIterator;
			firstTime = false;
		}
		else
		{
			if (((positionsIterator->x * positionsIterator->x) + (positionsIterator->y * positionsIterator->y)) < shortestSoFar)
			{
				shortestSoFar = ((positionsIterator->x * positionsIterator->x) + (positionsIterator->y * positionsIterator->y));
				shortest = positionsIterator;
			}
		}
	}

	for (PositionList::const_iterator positionsIterator = positions.begin(); positionsIterator != positions.end(); ++positionsIterator)
	{
		if (positionsIterator != shortest)
		{
			result.push_back(*positionsIterator);
		}
	}

	return result;
}

/*
 * calculateLength - calculates the total length of the movements from a list of relative movements.
 */

int calculateLength(const PositionList &positions)
{
	int result = 0;

	for (PositionList::const_iterator positionsIterator = positions.begin(); positionsIterator != positions.end(); ++positionsIterator)
	{
		if (positionsIterator->x > 0)
		{
			result += positionsIterator->x;
		}
		else if (positionsIterator->x < 0)
		{
			result -= positionsIterator->x;
		}
		else if (positionsIterator->y > 0)
		{
			result += positionsIterator->y;
		}
		else
		{
			result -= positionsIterator->y;
		}
	}

	return result;
}

}
