/* symbian_input_mon.h - AO to invoke callback on changes in input. 
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   roadmap_main.h
 */

#ifndef __SYMBIAN_INPUT_MON_H__
#define __SYMBIAN_INPUT_MON_H__

extern "C" {
#include "roadmap_main.h"
#include "roadmap_io.h"
}

#include <e32base.h>

typedef struct roadmap_main_io {
   RoadMapIO *io;
   RoadMapInput callback;
   int is_valid;
} roadmap_main_io;

class CIOMonitor : public CActive
{
public:
  CIOMonitor();
  virtual ~CIOMonitor();

  //  Initialize and start monitoring
  static CIOMonitor* StartL(RoadMapIO* apIO, void* apParam);

private:
  void ConstructL(RoadMapIO* apIO, void* apParam);
  virtual void RunL();
  virtual void DoCancel();

  RoadMapIO         *m_pIO;
  roadmap_main_io   *m_pData;
};

#endif  // __SYMBIAN_INPUT_MON_H__
