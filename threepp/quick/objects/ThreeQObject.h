//
// Created by byter on 12/12/17.
//

#ifndef THREEPPQ_THREEDOBJECT_H
#define THREEPPQ_THREEDOBJECT_H

#include <QObject>
#include <QQmlListProperty>
#include <QVector3D>
#include <QVector4D>
#include <threepp/quick/materials/Material.h>
#include <threepp/quick/Three.h>
#include <threepp/quick/objects/VertexNormalsHelper.h>

namespace three {
namespace quick {

class Scene;

class BoundingBox : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVector3D min READ min NOTIFY minChanged)
  Q_PROPERTY(QVector3D max READ max NOTIFY maxChanged)

  QVector3D _min, _max;

public:
  BoundingBox(const math::Box3 &bbox, QObject *parent=nullptr)
     : QObject(parent),
       _min(bbox.min().x(), bbox.min().y(), bbox.min().z()),
       _max(bbox.max().x(), bbox.max().y(), bbox.max().z())
  {}

  QVector3D min() const {return _min;}
  QVector3D max() const {return _max;}

signals:
  void minChanged();
  void maxChanged();
};

class ThreeQObject : public QObject
{
  friend class ObjectPicker;
  friend class Model;
  friend class ModelRef;

Q_OBJECT
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
  Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
  Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
  Q_PROPERTY(three::quick::Material * material READ material WRITE setMaterial NOTIFY materialChanged)
  Q_PROPERTY(three::quick::Material * customDistanceMaterial READ customDistanceMaterial WRITE setCustomDistanceMaterial NOTIFY customDistanceMaterialChanged)
  Q_PROPERTY(three::quick::Material * customDepthMaterial READ customDepthMaterial WRITE setCustomDepthMaterial NOTIFY customDepthMaterialChanged)
  Q_PROPERTY(bool castShadow READ castShadow WRITE setCastShadow NOTIFY castShadowChanged)
  Q_PROPERTY(bool receiveShadow READ receiveShadow WRITE setReceiveShadow NOTIFY receiveShadowChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY(bool matrixAutoUpdate READ matrixAutoUpdate WRITE setMatrixAutoUpdate NOTIFY matrixAutoUpdateChanged)
  Q_PROPERTY(three::quick::Three::GeometryType type READ geometryType WRITE setGeometryType NOTIFY geometryTypeChanged)
  Q_PROPERTY(BoundingBox *boundingBox READ boundingBox CONSTANT)
  Q_PROPERTY(VertexNormalsHelper *vertexNormals READ vertexNormals CONSTANT)
  Q_PROPERTY(QQmlListProperty<three::quick::ThreeQObject> children READ children)
  Q_CLASSINFO("DefaultProperty", "children")

protected:
  QString _name;

  TrackingProperty<QVector3D> _position {QVector3D(0, 0, 0)};

  TrackingProperty<QVector3D> _rotation {QVector3D(0, 0, 0)};

  TrackingProperty<QVector3D> _scale {QVector3D(1, 1, 1)};

  VertexNormalsHelper *_normalsHelper = nullptr;

  TrackingProperty<bool> _castShadow {false};
  TrackingProperty<bool> _receiveShadow {false};
  TrackingProperty<bool> _visible {true};
  TrackingProperty<bool> _matrixAutoUpdate {true};

  BoundingBox *_boundingBox = nullptr;

  QList<ThreeQObject *> _children;

  Three::GeometryType _geometryType = Three::LinearGeometry;

  Material *_material = nullptr, *_customDistanceMaterial = nullptr, *_customDepthMaterial = nullptr;

  three::Object3D::Ptr _object;
  three::Object3D::Ptr _parentObject;
  Scene *_scene = nullptr;

  virtual three::Object3D::Ptr _create() {return nullptr;}

  virtual three::Object3D::Ptr _copy(Object3D::Ptr copyable) {return nullptr;}

  virtual void _post_create() {}

  void init();

  ThreeQObject(QObject *parent = nullptr) : QObject(parent) {}
  ThreeQObject(three::Object3D::Ptr object, QObject *parent = nullptr)
     : QObject(parent), _object(object) {init();}
  ThreeQObject(three::Object3D::Ptr object, Material *material, QObject *parent = nullptr)
     : QObject(parent), _object(object), _material(material) {init();}

  virtual void updateMaterial() {
    _object->setMaterial(_material->getMaterial());
  }

  static void append_child(QQmlListProperty<ThreeQObject> *list, ThreeQObject *obj);
  static int count_children(QQmlListProperty<ThreeQObject> *);
  static ThreeQObject *child_at(QQmlListProperty<ThreeQObject> *, int);
  static void clear_children(QQmlListProperty<ThreeQObject> *);

  QQmlListProperty<ThreeQObject> children();

  void setObject(const three::Object3D::Ptr &object);

  void recreate();

public:
  enum class ObjectState {Removed, Added};
  Signal<void(Object3D::Ptr created, ObjectState state)> onObjectChanged;
  using OnObjectChangedId = decltype(onObjectChanged)::ConnectionId;

  ~ThreeQObject() override {
    if(_normalsHelper) _normalsHelper->deleteLater();
  }

  QVector3D position() const {return _position;}
  QVector3D rotation() const {return _rotation;}
  QVector3D scale() const {return _scale;}

  VertexNormalsHelper *vertexNormals();

  BoundingBox *boundingBox();

  void setPosition(const QVector3D &position, bool propagate=true);

  void setRotation(const QVector3D &rotation, bool propagate=true);

  void setScale(QVector3D scale, bool propagate=true);

  Material *material() const {return _material;}

  void setMaterial(Material *material, bool update=true);

  Material *customDistanceMaterial() const {return _customDistanceMaterial;}

  void setCustomDistanceMaterial(Material *material, bool update=true);

  Material *customDepthMaterial() const {return _customDepthMaterial;}

  void setCustomDepthMaterial(Material *material, bool update=true);

  bool matrixAutoUpdate() const {return _object ? _object->matrixAutoUpdate : _matrixAutoUpdate;}

  void setMatrixAutoUpdate(bool matrixAutoUpdate, bool propagate=true);

  bool castShadow() const {return _object ? _object->castShadow : _castShadow;}

  void setCastShadow(bool castShadow, bool propagate=true);

  bool receiveShadow() const {return _object ? _object->receiveShadow : _receiveShadow;}

  void setReceiveShadow(bool receiveShadow, bool propagate=true);

  bool visible() const {return _object ? _object->visible() : _visible;}

  void setVisible(bool visible, bool propagate=true);

  const QString &name() const {return _name;}

  void setName(const QString &name, bool propagate=true);

  Three::GeometryType geometryType() const {return _geometryType;}

  void setGeometryType(Three::GeometryType geometryType);

  three::Object3D::Ptr object() const {return _object;}

  three::Object3D::Ptr create(quick::Scene *scene, Object3D::Ptr parent);

  /**
   * mark all tracking properties as 'not externally set'
   */
  virtual void unset();

  Q_INVOKABLE void copy(ThreeQObject *copyable);
  Q_INVOKABLE QVariant parentObject(QString name);
  Q_INVOKABLE void add(three::quick::ThreeQObject *object);
  Q_INVOKABLE void remove(three::quick::ThreeQObject *object);
  Q_INVOKABLE QVector3D worldPosition() const;
  Q_INVOKABLE void clear();
  Q_INVOKABLE QObject *alone();
  Q_INVOKABLE void updateMaterials();

  Q_INVOKABLE void rotateX(float angle);
  Q_INVOKABLE void rotateY(float angle);
  Q_INVOKABLE void rotateZ(float angle);
  Q_INVOKABLE void translateZ(float distance);
  Q_INVOKABLE void lookAt(const QVector3D &position);

signals:
  void positionChanged();
  void rotationChanged();
  void scaleChanged();
  void materialChanged();
  void customDistanceMaterialChanged();
  void customDepthMaterialChanged();
  void castShadowChanged();
  void receiveShadowChanged();
  void visibleChanged();
  void matrixAutoUpdateChanged();
  void nameChanged();
  void geometryTypeChanged();
  void objectCreated();
};

}
}
Q_DECLARE_METATYPE(three::quick::ThreeQObject *)

#endif //THREEPPQ_THREEDOBJECT_H
