/****************************************************************************
 * This file handles the Stereoscopic 3D shutter-glasses of Famicom 3D System
 * They worked the way a modern 3D TV works, but at a much lower refresh rate
 * All we do here is set a global variable to the state of the OUT1 pin
 ****************************************************************************/

/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2010 Carl Kenner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "share.h"

uint8 shutter_3d;

static void Write(uint8 V)
{
  shutter_3d = (V >> 1) & 1;
}

static INPUTCFC Famicom3D={0,Write,0,0,0,0};

INPUTCFC *FCEU_InitFamicom3D(void)
{
  return(&Famicom3D);
}

