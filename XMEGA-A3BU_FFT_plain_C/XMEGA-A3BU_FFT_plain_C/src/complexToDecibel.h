/************************************************************************
	complextodecibel.h

    16-Bit Fast Hartley Transform
	Functions to convert linear complex results into real dB results
    Copyright (C) 2013 Simon Inns

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

	Email: simon.inns@gmail.com (please use the forum for questions)
	Forum: http://www.waitingforfriday.com/forum

************************************************************************/

#ifndef COMPLEXTODECIBEL_H_
#define COMPLEXTODECIBEL_H_

// External function prototypes
void complexToDecibel(int16_t *fx);
void complexToDecibelWithGain(int16_t *fx);

#endif /* COMPLEXTODECIBEL_H_ */
