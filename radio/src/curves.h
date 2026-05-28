/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

enum BaseCurves {
  CURVE_NONE,
  CURVE_X_GT0,
  CURVE_X_LT0,
  CURVE_ABS_X,
  CURVE_F_GT0,
  CURVE_F_LT0,
  CURVE_ABS_F,
  CURVE_BASE
};

#if defined(CURVES)
void curveClear(uint8_t index);
void curveMirror(uint8_t index);
bool isCurveUsed(uint8_t index);
void loadCurves();
int8_t * curveAddress(uint8_t idx);
bool moveCurve(uint8_t index, int8_t shift);
int8_t getCurveX(int noPoints, int point);
void resetCustomCurveX(int8_t * points, int noPoints);
point_t getPoint(uint8_t i);
point_t getPoint(uint8_t curveIndex, uint8_t index);
int applyCustomCurve(int x, uint8_t idx);
int applyCurve(int x, CurveRef & curve);
int applyCurrentCurve(int x);

char *getCurveRefString(char *dest, size_t len, const CurveRef& curve);

#else

inline void curveClear(uint8_t index) {}
inline void curveMirror(uint8_t index) {}
inline bool isCurveUsed(uint8_t index) { return false; }
inline void loadCurves() {}
inline int8_t * curveAddress(uint8_t idx) { return nullptr; }
inline bool moveCurve(uint8_t index, int8_t shift) { return false; }
inline int8_t getCurveX(int noPoints, int point) { return 0; }
inline void resetCustomCurveX(int8_t * points, int noPoints) {}
inline point_t getPoint(uint8_t i) { return {0, 0}; }
inline point_t getPoint(uint8_t curveIndex, uint8_t index) { return {0, 0}; }
inline int applyCustomCurve(int x, uint8_t idx) { return x; }
inline int applyCurve(int x, CurveRef & curve) { return x; }
inline int applyCurrentCurve(int x) { return x; } 
#endif