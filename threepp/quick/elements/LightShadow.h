//
// Created by byter on 12/20/17.
//

#ifndef THREEPPQ_LIGHTSHADOW_H
#define THREEPPQ_LIGHTSHADOW_H

#include <QObject>
#include <QSize>
#include <threepp/quick/cameras/PerspectiveCamera.h>
#include <threepp/quick/cameras/OrtographicCamera.h>

namespace three {
namespace quick {

class LightShadow : public QObject
{
Q_OBJECT
  Q_PROPERTY(QSize mapSize READ mapSize WRITE setMapSize NOTIFY mapSizeChanged)
  Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)

  QSize _mapSize {512, 512};
  qreal _radius = 1;

public:
  LightShadow(QObject *parent=nullptr) : QObject(parent) {}

  QSize mapSize() {return _mapSize;}

  void setMapSize(QSize size) {
    if(_mapSize != size) {
      _mapSize = size;
      emit mapSizeChanged();
    }
  }

  qreal radius() {return _radius;}

  void setRadius(qreal radius) {
    if(_radius != radius) {
      _radius = radius;
      emit radiusChanged();
    }
  }

signals:
  void mapSizeChanged();
  void radiusChanged();
};

class LightShadowPC : public LightShadow
{
Q_OBJECT
  Q_PROPERTY(PerspectiveCamera *camera READ camera CONSTANT)

  PerspectiveCamera _camera;

public:
  PerspectiveCamera *camera() {
    return &_camera;
  }
};

class LightShadowOC : public LightShadow
{
Q_OBJECT
  Q_PROPERTY(OrtographicCamera *camera READ camera CONSTANT)

  OrtographicCamera _camera;

public:
  OrtographicCamera *camera() {
    return &_camera;
  }
};

}
}

#endif //THREEPPQ_LIGHTSHADOW_H
