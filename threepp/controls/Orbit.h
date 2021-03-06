//
// Created by byter on 12/24/17.
//

#ifndef THREEPP_ORBITCONTROLS_H
#define THREEPP_ORBITCONTROLS_H

#include <threepp/util/osdecl.h>
#include <threepp/camera/Camera.h>
#include <threepp/math/Spherical.h>
#include <threepp/util/simplesignal.h>

//var controls = new THREE.OrbitControls( camera, renderer.domElement );

namespace three {
namespace control {

/**
 * @author qiao / https://github.com/qiao
 * @author mrdoob / http://mrdoob.com
 * @author alteredq / http://alteredqualia.com/
 * @author WestLangley / http://github.com/WestLangley
 * @author erich666 / http://erichaines.com
 */

// This set of controls performs orbiting, dollying (zooming), and panning.
// Unlike TrackballControls, it maintains the "up" direction object.up (+Y by default).
//
class DLX Orbit
{
public:
  enum class State
  {
    NONE = -1, ROTATE = 0, DOLLY = 1, PAN = 2, TOUCH_ROTATE = 3, TOUCH_DOLLY = 4, TOUCH_PAN = 5
  };

protected:
  class Impl;
  class PerspectiveImpl;
  class OrthographicImpl;
  Impl *_impl;
  Camera::Ptr _camera;

  // for reset
  math::Vector3 _target0;
  math::Vector3 _position0;
  float _zoom0;

  State _state = State::NONE;

  // current position in spherical coordinates
  math::Spherical _spherical;
  math::Spherical _sphericalDelta;

  float scale = 1;
  math::Vector3 _panOffset;
  bool _zoomChanged = false;

  math::Vector2 _rotateStart;
  math::Vector2 _rotateEnd;
  math::Vector2 _rotateDelta;

  math::Vector2 _panStart;
  math::Vector2 _panEnd;
  math::Vector2 _panDelta;

  math::Vector2 _dollyStart;
  math::Vector2 _dollyEnd;
  math::Vector2 _dollyDelta;

  math::Vector3 _offset;

  // so camera.up is the orbit axis
  math::Quaternion _quat;
  math::Quaternion _quatInverse;

  math::Vector3 lastPosition;
  math::Quaternion lastQuaternion;

  static Impl *makeImpl(Orbit &orbit, Camera::Ptr camera);

  Orbit(Camera::Ptr camera, const math::Vector3 &target);

  virtual unsigned clientWidth() = 0;

  virtual unsigned clientHeight() = 0;

  // deltaX and deltaY are in pixels; right and down are positive
  void _pan(float deltaX, float deltaY);

  void _dollyIn(float dollyScale);

  void _dollyOut(float dollyScale);

  bool update();

  void startRotate(unsigned x, unsigned y);

  void startZoom(unsigned x, unsigned y);

  void startPan(unsigned x, unsigned y);

  void doRotate(float x, float y);

  void doDolly(unsigned x, unsigned y);

  void doPan(unsigned x, unsigned y);

public:
  ~Orbit();
  using Ptr = std::shared_ptr<Orbit>;

  Signal<void(State)> onChanged;

  /**
   * @param deltaX X rotation in screen pixels, positive value moves right
   * @param deltaY Y rotation in screen pixels, positive value moves down
   */
  void rotate(float deltaX, float deltaY);

  /**
   * @param deltaX X movement in screen pixels, positive value moves right
   * @param deltaY Y movement in screen pixels, positive value moves down
   */
  void pan(float deltaX, float deltaY);

  /**
   * increase the zoom by a factor
   * @param factor
   */
  void zoomIn(float factor) {
    _dollyIn(factor);
  }

  /**
   * decrease the zoom by a factor
   * @param factor
   */
  void zoomOut(float factor) {
    _dollyOut(factor);
  }

  /**
   * update the camera to make the given object completely visible
   * @param object
   */
  void showAll(Object3D::Ptr object);

  // "target" sets the location of focus, where the object orbits around
  math::Vector3 target;

  // How far you can dolly in and out ( PerspectiveCamera only )
  float minDistance = 0;
  float maxDistance = std::numeric_limits<float>::infinity();

  // How far you can zoom in and out ( OrthographicCamera only )
  float minZoom = 0;
  float maxZoom = std::numeric_limits<float>::infinity();

  // How far you can orbit vertically, upper and lower limits.
  // Range is 0 to Math.PI radians.
  float minPolarAngle = 0; // radians
  float maxPolarAngle = (float) M_PI; // radians

  // How far you can orbit horizontally, upper and lower limits.
  // If set, must be a sub-interval of the interval [ - Math.PI, Math.PI ].
  float minAzimuthAngle = -std::numeric_limits<float>::infinity(); // radians
  float maxAzimuthAngle = std::numeric_limits<float>::infinity(); // radians

  // Set to true to enable damping (inertia)
  // If damping is enabled, you must call controls.update() in your animation loop
  bool enableDamping = false;
  float dampingFactor = 0.25f;

  // This option actually enables dollying in and out; left as "zoom" for backwards compatibility.
  // Set to false to disable zooming
  bool enableZoom = true;
  float zoomSpeed = 1.0f;

  // Set to false to disable rotating
  bool enableRotate = true;
  float rotateSpeed = 1.0f;

  // Set to false to disable panning
  bool enablePan = true;
  float keyPanSpeed = 7.0f;  // pixels moved per arrow key push

  // Set to true to automatically rotate around the target
  // If auto-rotate is enabled, you must call controls.update() in your animation loop
  bool autoRotate = false;
  float autoRotateSpeed = 2.0f; // 30 seconds per round when fps is 60

  float getPolarAngle() const
  {
    return _spherical.phi();
  }

  float getAzimuthalAngle() const
  {
    return _spherical.theta();
  }

  float getAutoRotationAngle()
  {
    return 2.0f * (float) M_PI / 60 / 60 * autoRotateSpeed;
  }

  float getZoomScale()
  {
    return std::pow(0.95f, zoomSpeed);
  }

  void rotateLeft(float angle)
  {
    _sphericalDelta.theta() -= angle;
  }

  void rotateUp(float angle)
  {
    _sphericalDelta.phi() -= angle;
  }

  void panLeft(float distance, const math::Matrix4 &objectMatrix)
  {
    math::Vector3 v = math::Vector3::fromMatrixColumn(objectMatrix, 0); // get X column of objectMatrix
    v *= -distance;

    _panOffset += v;
  }

  void panUp(float distance, const math::Matrix4 &objectMatrix)
  {
    math::Vector3 v = math::Vector3::fromMatrixColumn(objectMatrix, 1); // get Y column of objectMatrix
    v *= distance;

    _panOffset += v;
  }

  float getDistance() const;

  void set(float polar, float azimuth);

  void saveState();

  virtual void reset();

  bool handleMove(unsigned x, unsigned y);
  bool handleDelta(int delta);
  bool resetState();
};

}
}

#endif //THREEPP_ORBITCONTROLS_H
