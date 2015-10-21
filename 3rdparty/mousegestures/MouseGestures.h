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

#ifndef MOUSEGESTURES_H
#define MOUSEGESTURES_H

#include <list>

namespace MouseGestures
{

/*
 * The available actions.
 */

enum MouseAction
{
	UnknownMouseAction,
	MoveUpMouseAction,
	MoveDownMouseAction,
	MoveLeftMouseAction,
	MoveRightMouseAction,
	MoveHorizontallyMouseAction,
	MoveVerticallyMouseAction
};

/*
 * A list of actions.
 */

typedef std::list<MouseAction> ActionList;

class Recognizer
{

public:
	explicit Recognizer(int minimumMovement = 5, double minimumMatch = 0.9);
	Recognizer(const Recognizer &other);
	~Recognizer();

	void startGesture(int x, int y);
	void addPosition(int x, int y);
	int registerGesture(const ActionList &actions);
	int endGesture();
	int recognizeGesture();

private:
	struct Private;

	Private *d;
};

}

#endif
