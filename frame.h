/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef _FRAME_H
#define _FRAME_H

#include <list>
#include "swftypes.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class ControlTag;
class IDisplayListElem;
class MovieClip;

class PlaceInfo
{
public:
	MATRIX Matrix;
};

class Frame
{
private:
	STRING Label;
	IFunction* script;
	bool initialized;
public:
	std::list<DisplayListTag*> blueprint;
	std::list<std::pair<PlaceInfo, IDisplayListElem*> > displayList;
	//A temporary vector for control tags
	std::vector < ControlTag* > controls;
	Frame():script(NULL),initialized(false){}
	~Frame();
	void Render();
	void setLabel(STRING l);
	void setScript(IFunction* s){script=s;}
	void runScript();
	void init(MovieClip* parent, std::list < std::pair<PlaceInfo, IDisplayListElem*> >& d);
	bool isInitialized() const { return initialized; }
};
};

#endif