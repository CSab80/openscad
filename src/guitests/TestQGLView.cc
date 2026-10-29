#include "TestQGLView.h"

#include <cmath>
#include <limits>
#include <QApplication>
#include <QNativeGestureEvent>
#include <QSignalSpy>
#include <QTest>

#include "gui/QGLView.h"
#include "utils/scope_guard.hpp"

namespace {

void sendGesture(QGLView *view, Qt::NativeGestureType type, double value, const QPointF& pos)
{
  // QWidgetWindow forwards native gestures with window-relative local coordinates.
  const QPointF windowPos = view->mapTo(view->window(), pos.toPoint());
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  QNativeGestureEvent event(type, QPointingDevice::primaryPointingDevice(), 2, windowPos, windowPos,
                            view->mapToGlobal(pos.toPoint()), value, QPointF());
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QNativeGestureEvent event(type, QPointingDevice::primaryPointingDevice(), windowPos, windowPos,
                            view->mapToGlobal(pos.toPoint()), value, 0, 0);
#else
  QNativeGestureEvent event(type, nullptr, windowPos, windowPos, view->mapToGlobal(pos.toPoint()), value,
                            0, 0);
#endif
  QApplication::sendEvent(view, &event);
}

}  // namespace

void TestQGLView::pinchZoom_data()
{
  QTest::addColumn<bool>("mouseCentric");
  QTest::addColumn<bool>("orthogonal");
  QTest::newRow("perspective-centered") << false << false;
  QTest::newRow("perspective-cursor") << true << false;
  QTest::newRow("orthogonal-centered") << false << true;
  QTest::newRow("orthogonal-cursor") << true << true;
}

void TestQGLView::pinchZoom()
{
  QFETCH(bool, mouseCentric);
  QFETCH(bool, orthogonal);
  auto *view = window->findChild<QGLView *>();
  QVERIFY(view);
  const auto originalCamera = view->cam;
  view->resetView();
  view->setOrthoMode(orthogonal);
  view->setMouseCentricZoom(mouseCentric);
  const auto initialDistance = view->cam.zoomValue();
  const auto initialTranslation = view->cam.object_trans;
  const auto initialRotation = view->cam.object_rot;
  const auto initialFov = view->cam.fovValue();
  const QPointF pos(view->width() * 0.75, view->height() * 0.25);
  QSignalSpy changed(view, &QGLView::cameraChanged);

  // Spreading fingers increases the model's apparent size by exactly 25%.
  sendGesture(view, Qt::ZoomNativeGesture, 0.25, pos);
  QVERIFY(std::abs(view->cam.zoomValue() - initialDistance / 1.25) < 1e-10);
  if (mouseCentric) {
    QVERIFY((view->cam.object_trans - initialTranslation).norm() > 0.0);
  } else {
    QVERIFY(view->cam.object_trans.isApprox(initialTranslation));
  }
  QVERIFY(view->cam.object_rot.isApprox(initialRotation));
  QCOMPARE(view->cam.fovValue(), initialFov);

  // The reciprocal pinch returns to the same distance and cursor anchor.
  sendGesture(view, Qt::ZoomNativeGesture, -0.2, pos);
  QVERIFY(std::abs(view->cam.zoomValue() - initialDistance) < 1e-10);
  QVERIFY((view->cam.object_trans - initialTranslation).norm() < 1e-10);
  QCOMPARE(changed.count(), 2);

  // Small trackpad deltas must accumulate rather than disappear through integer rounding.
  for (int i = 0; i < 100; ++i) {
    sendGesture(view, Qt::ZoomNativeGesture, 0.0001, pos);
  }
  QVERIFY(std::abs(view->cam.zoomValue() - initialDistance / std::pow(1.0001, 100)) < 1e-9);
  QCOMPARE(changed.count(), 102);
  view->setCamera(originalCamera);
  view->setMouseCentricZoom(true);
}

void TestQGLView::pinchSequence()
{
  auto *view = window->findChild<QGLView *>();
  QVERIFY(view);
  const auto originalCamera = view->cam;
  const QPointF pos(view->width() / 2.0, view->height() / 2.0);
  QSignalSpy changed(view, &QGLView::cameraChanged);
  sendGesture(view, Qt::BeginNativeGesture, 0.0, pos);
  sendGesture(view, Qt::RotateNativeGesture, 30.0, pos);
  sendGesture(view, Qt::ZoomNativeGesture, 0.0, pos);
  sendGesture(view, Qt::ZoomNativeGesture, -1.0, pos);
  sendGesture(view, Qt::ZoomNativeGesture, -2.0, pos);
  sendGesture(view, Qt::ZoomNativeGesture, std::numeric_limits<double>::infinity(), pos);
  sendGesture(view, Qt::ZoomNativeGesture, std::numeric_limits<double>::quiet_NaN(), pos);
  sendGesture(view, Qt::EndNativeGesture, 0.0, pos);
  QCOMPARE(view->cam.zoomValue(), originalCamera.zoomValue());
  QVERIFY(view->cam.object_trans.isApprox(originalCamera.object_trans));
  QVERIFY(view->cam.object_rot.isApprox(originalCamera.object_rot));
  QCOMPARE(changed.count(), 0);
}

void TestQGLView::wheelZoom_data()
{
  pinchZoom_data();
}

void TestQGLView::pinchAnchor_data()
{
  QTest::addColumn<bool>("orthogonal");
  QTest::addColumn<QPointF>("relativePosition");
  QTest::newRow("perspective-center") << false << QPointF(0.5, 0.5);
  QTest::newRow("perspective-top-right") << false << QPointF(0.8, 0.2);
  QTest::newRow("perspective-bottom-left") << false << QPointF(0.2, 0.8);
  QTest::newRow("orthogonal-center") << true << QPointF(0.5, 0.5);
  QTest::newRow("orthogonal-top-right") << true << QPointF(0.8, 0.2);
  QTest::newRow("orthogonal-bottom-left") << true << QPointF(0.2, 0.8);
}

void TestQGLView::pinchAnchor()
{
  QFETCH(bool, orthogonal);
  QFETCH(QPointF, relativePosition);
  auto *view = window->findChild<QGLView *>();
  QVERIFY(view);
  const auto originalCamera = view->cam;
  auto *originalContext = getGLContext();
  view->makeCurrent();
  auto restore = sg::make_scope_guard([view, originalCamera, originalContext] {
    view->setCamera(originalCamera);
    view->setMouseCentricZoom(true);
    view->doneCurrent();
    setGLContext(originalContext);
  });
  view->resetView();
  view->setOrthoMode(orthogonal);
  view->setMouseCentricZoom(true);
  view->setupCamera();

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  // Choose a point on the camera's target plane directly beneath the pointer.
  GLdouble centerX, centerY, depth;
  QVERIFY(gluProject(0, 0, 0, view->modelview, view->projection, viewport, &centerX, &centerY, &depth) ==
          GL_TRUE);
  const QPoint pos(view->width() * relativePosition.x(), view->height() * relativePosition.y());
  const auto screenX = viewport[0] + (pos.x() + 0.5) * viewport[2] / view->width();
  const auto screenY = viewport[1] + viewport[3] - (pos.y() + 0.5) * viewport[3] / view->height();
  GLdouble worldX, worldY, worldZ;
  QVERIFY(gluUnProject(screenX, screenY, depth, view->modelview, view->projection, viewport, &worldX,
                       &worldY, &worldZ) == GL_TRUE);

  for (const auto magnification : {0.25, -0.2}) {
    sendGesture(view, Qt::ZoomNativeGesture, magnification, pos);
    view->setupCamera();
    GLdouble projectedX, projectedY, projectedZ;
    QVERIFY(gluProject(worldX, worldY, worldZ, view->modelview, view->projection, viewport, &projectedX,
                       &projectedY, &projectedZ) == GL_TRUE);
    // Verify the rendered position, independently of the zoom translation formula.
    // Allow a hundredth of a pixel for the OpenGL matrices' floating-point precision.
    const auto details =
      QString("Anchor drift in pixels: [%1, %2]").arg(projectedX - screenX).arg(projectedY - screenY);
    QVERIFY2(std::abs(projectedX - screenX) < 0.01, qPrintable(details));
    QVERIFY2(std::abs(projectedY - screenY) < 0.01, qPrintable(details));
  }
}

void TestQGLView::wheelZoom()
{
  QFETCH(bool, mouseCentric);
  QFETCH(bool, orthogonal);
  auto *view = window->findChild<QGLView *>();
  QVERIFY(view);
  const auto originalCamera = view->cam;
  view->resetView();
  view->setOrthoMode(orthogonal);
  view->setMouseCentricZoom(mouseCentric);
  const auto initialDistance = view->cam.zoomValue();
  const QPointF pos(view->width() * 0.75, view->height() * 0.25);
  const auto globalPos = view->mapToGlobal(pos.toPoint());
  QWheelEvent zoomIn(pos, globalPos, QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                     Qt::NoScrollPhase, false);
  QApplication::sendEvent(view, &zoomIn);
  QVERIFY(std::abs(view->cam.zoomValue() - initialDistance * 0.9) < 1e-10);
  QWheelEvent zoomOut(pos, globalPos, QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
  QApplication::sendEvent(view, &zoomOut);
  QVERIFY(std::abs(view->cam.zoomValue() - initialDistance) < 1e-10);
  QVERIFY(view->cam.object_trans.norm() < 1e-10);
  view->setCamera(originalCamera);
  view->setMouseCentricZoom(true);
}
