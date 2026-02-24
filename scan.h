/* scan.h - Header do Scanner */

#ifndef SCAN_H
#define SCAN_H

#include "globals.h"

#define MAXTOKENLEN 40

extern char stringToken[MAXTOKENLEN + 1];

TokenType getToken(void);

#endif