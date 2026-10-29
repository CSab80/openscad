#pragma once

#include "UXTest.h"

class TestQGLView : public UXTest
{
  Q_OBJECT;
private slots:
  void pinchZoom_data();
  void pinchZoom();
  void pinchSequence();
  void pinchAnchor_data();
  void pinchAnchor();
  void wheelZoom_data();
  void wheelZoom();
};
