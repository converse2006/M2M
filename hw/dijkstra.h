/*
 * Copyright (C) 2012 PASLab CSIE NTU. All rights reserved.
 *      - Chen Chun-Han <converse2006@gmail.com>
 */

#ifndef _DIJKSTRA_H_
#define _DIJKSTRA_H_

#include "m2m.h"
extern  void printD(int id);

extern void clearPath();

extern void routePath(int dest);

extern void printPath(int start_node);

extern void dijkstra(int s);

#endif
