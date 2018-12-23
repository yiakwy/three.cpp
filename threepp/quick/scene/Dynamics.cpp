//
// Created by byter on 11/9/18.
//

#include "Dynamics.h"
#include "Scene.h"
#include <threepp/quick/loader/FileSystemLoader.h>
#include <threepp/quick/loader/QtResourceLoader.h>
#include <threepp/util/impl/utils.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace three {
namespace quick {

using namespace math;

struct PropellerHinge : public Hinge
{
  Object3D::Ptr propeller;
  Vector3 point;
  Vector3 center;
  Vector3 centerWorld;
  Vector3 axis;
  Vector3 axisWorld;

  void update(const Quaternion &)
  {
    centerWorld = propeller->localToWorld(center);
    axisWorld = (propeller->localToWorld(point) - propeller->localToWorld(center)).normalized();
  }

  PropellerHinge(const std::string &name, Object3D::Ptr propeller, const Vector3 &point) : Hinge(name), propeller(propeller),
                                                                                           point(point)
  {
    continuous = true;
    const auto box = propeller->computeBoundingBox();

    center = propeller->worldToLocal(box.getCenter());
    axis = (point - center).normalized();

    update(propeller->quaternion());

    propeller->quaternion().onChange.connect(this, &PropellerHinge::update);
  }

  void rotate(float angle) const override
  {
    auto wpos = propeller->parent()->localToWorld(propeller->position());
    wpos -= centerWorld;
    wpos.apply(axisWorld, angle);
    wpos += centerWorld;
    propeller->position() = propeller->parent()->worldToLocal(wpos);

    propeller->rotateOnAxis(axis, angle);
    propeller->updateMatrix();
  }

  void save(QJsonObject &json) const override
  {
    json["type"] = "propeller";
    json["name"] = QString::fromStdString(name);
    json["object"] = QString::fromStdString(propeller->name());

    QJsonObject pointObject;
    pointObject["x"] = point.x();
    pointObject["y"] = point.y();
    pointObject["z"] = point.z();
    json["point"] = pointObject;

    QJsonObject centerObject;
    centerObject["x"] = center.x();
    centerObject["y"] = center.y();
    centerObject["z"] = center.z();
    json["center"] = centerObject;
  }

  using Ptr = std::shared_ptr<PropellerHinge>;
  static Ptr load(Object3D::Ptr object, const QJsonObject &json)
  {
    std::string name = json["name"].toString().toStdString();

    const QString propellerName = json["propeller"].toString();
    const auto an = propellerName.toStdString();
    Object3D::Ptr propeller = an == object->name() ? object : object->getChildByName(an);

    const QJsonObject pointObject = json["back"].toObject();
    Vector3 point;
    point.set(pointObject["x"].toDouble(), pointObject["y"].toDouble(), pointObject["z"].toDouble());

    return PropellerHinge::make(name, propeller, point);
  }

  static Ptr make(const std::string &name, Object3D::Ptr object, const Vector3 &point)
  {
    return Ptr(new PropellerHinge(name, object, point));
  }
};

struct DoorHinge : public Hinge
{
  Object3D::Ptr anchor;
  Object3D::Ptr element;

  Direction direction;
  Vector3 axisWorld;
  Vector3 pointWorld;

  Vector3 axisLocal;
  Vector3 point1;
  Vector3 point2;

  DoorHinge(const std::string &name, Object3D::Ptr anchor, Object3D::Ptr element, const Vector3 &point1, const Vector3 &point2,
            float angleLimit)
     : Hinge(name, angleLimit), anchor(anchor), element(element), point1(point1), point2(point2)
  {
    setup();
  }

  void setup()
  {
    //calculate hinge axis and hinge point
    const auto hp1 = anchor->localToWorld(point1);
    const auto hp2 = anchor->localToWorld(point2);

    axisWorld = (hp1 - hp2).normalized();
    axisLocal = (element->worldToLocal(hp1) - element->worldToLocal(hp2)).normalized();
    pointWorld = (hp1 + hp2) * 0.5;

    //calculate position of door to determine turning direction
    const auto hingePoint = (point1 + point2) * 0.5;

    const auto anchorCenter = anchor->computeBoundingBox().getCenter();
    const auto elementCenter = element->computeBoundingBox().getCenter();

    const auto anchorToHinge = (hingePoint - anchor->worldToLocal(anchorCenter)).normalized();
    const auto elementToHinge = (hingePoint - anchor->worldToLocal(elementCenter)).normalized();

    const auto crossVal = cross(anchorToHinge, elementToHinge);
    float leftRight = dot(crossVal, point1 - point2);

    direction = leftRight < 0.0f ? Hinge::Direction::CLOCKWISE : Hinge::Direction::COUNTERCLOCKWISE;
  }

  void rotate(float angle) const override
  {
    float theta = direction == Direction::CLOCKWISE ? -angle : angle;

    auto wpos = element->parent()->localToWorld(element->position());
    wpos -= pointWorld;
    wpos.apply(axisWorld, theta);
    wpos += pointWorld;
    element->position() = element->parent()->worldToLocal(wpos);

    element->rotateOnAxis(axisLocal, theta);
    element->updateMatrix();
  }

  void save(QJsonObject &json) const override
  {
    json["type"] = "door";
    json["name"] = QString::fromStdString(element->name());
    json["anchor"] = QString::fromStdString(anchor->name());
    json["element"] = QString::fromStdString(element->name());

    QJsonObject point1Object;
    point1Object["x"] = point1.x();
    point1Object["y"] = point1.y();
    point1Object["z"] = point1.z();
    json["point1"] = point1Object;

    QJsonObject point2Object;
    point2Object["x"] = point2.x();
    point2Object["y"] = point2.y();
    point2Object["z"] = point2.z();
    json["point2"] = point2Object;
  }

  using Ptr = std::shared_ptr<DoorHinge>;

  static Ptr load(Object3D::Ptr object, const QJsonObject &json)
  {
    std::string name = json["name"].toString().toStdString();

    const QString anchorName = json["anchor"].toString();
    const auto an = anchorName.toStdString();
    Object3D::Ptr anchor = an == object->name() ? object : object->getChildByName(an);

    const QString elementName = json["element"].toString();
    const auto en = elementName.toStdString();
    Object3D::Ptr element = en == object->name() ? object : object->getChildByName(en);

    if (!element || !anchor) {

      qCritical() << "Hinge definition does not match model: " << (!element ? elementName : anchorName)
                  << "not found";
      return nullptr;
    }

    const QJsonObject pointObject = json["point1"].toObject();
    Vector3 point1(pointObject["x"].toDouble(),
                   pointObject["y"].toDouble(),
                   pointObject["z"].toDouble());

    const QJsonObject axisObject = json["point2"].toObject();
    Vector3 point2(axisObject["x"].toDouble(),
                   axisObject["y"].toDouble(),
                   axisObject["z"].toDouble());
    float angleLimit = json["angleLimit"].toDouble();

    return make(name, anchor, element, point1, point2, angleLimit);
  }

  static Ptr make(const std::string &name, Object3D::Ptr anchor, Object3D::Ptr element,
                  const Vector3 &point1, const Vector3 &point2, float angleLimit)
  {
    return Ptr(new DoorHinge(name, anchor, element, point1, point2, angleLimit));
  }
};

struct WheelHinge : public Hinge
{
  enum class Position {FRONTLEFT, FRONTRIGHT, BACKLEFT, BACKRIGHT};

  Object3D::Ptr leftWheel;
  Object3D::Ptr rightWheel;
  Object3D *parent = nullptr;

  Vector3 parentCenter;
  Vector3 leftCenter;
  Vector3 rightCenter;
  Vector3 front;

  Vector3 leftAxis;
  Vector3 rightAxis;

  Position pos;
  int dir = 1;

  Vector3 leftCenterWorld;
  Vector3 rightCenterWorld;
  Vector3 leftAxisWorld;
  Vector3 rightAxisWorld;

  void save(QJsonObject &json) const override
  {
    json["type"] = "wheel";
    json["name"] = QString::fromStdString(name);
    json["parent"] = QString::fromStdString(parent->name());
    json["left"] = QString::fromStdString(leftWheel->name());
    json["right"] = QString::fromStdString(rightWheel->name());

    QJsonObject frontObject;
    frontObject["x"] = front.x();
    frontObject["y"] = front.y();
    frontObject["z"] = front.z();
    json["leftCenter"] = frontObject;
  }

  void rotate(float theta, Object3D::Ptr wheel, const Vector3 &axis, const Vector3 &axisWorld, const Vector3 &centerWorld) const
  {
    auto wpos = wheel->parent()->localToWorld(wheel->position());
    wpos -= centerWorld;
    wpos.apply(axisWorld, theta);
    wpos += centerWorld;
    wheel->position() = wheel->parent()->worldToLocal(wpos);

    wheel->rotateOnAxis(axis, theta);
    wheel->updateMatrix();
  }
  
  void rotate(float angle) const override
  {
    rotate(angle * dir, leftWheel, leftAxis, leftAxisWorld, leftCenterWorld);
    rotate(-angle * dir, rightWheel, rightAxis, rightAxisWorld, rightCenterWorld);
  }

  const char *toString(WheelHinge::Position pos)
  {
    switch(pos) {
      case WheelHinge::Position::BACKLEFT: return "BACKLEFT";
      case WheelHinge::Position::BACKRIGHT: return "BACKRIGHT";
      case WheelHinge::Position::FRONTLEFT: return "FRONTLEFT";
      case WheelHinge::Position::FRONTRIGHT: return "FRONTRIGHT";
      default: return "unknown";
    }
  }

  void update(const Quaternion &)
  {
    leftCenterWorld = leftWheel->localToWorld(leftCenter);
    rightCenterWorld = rightWheel->localToWorld(rightCenter);

    leftAxisWorld = (leftCenterWorld - rightCenterWorld).normalized();
    rightAxisWorld = (rightCenterWorld - leftCenterWorld).normalized();
  }

  WheelHinge(const std::string &name, Object3D::Ptr leftWheel, Object3D::Ptr rightWheel, const Vector3 &front, Object3D *parent)
     : Hinge(name), leftWheel(leftWheel), rightWheel(rightWheel), front(front.x(), front.y(), front.z()), parent(parent)
  {
    upm = 100;
    continuous = true;

    //save wheel midpoints as local coordinates
    Box3 leftBox = leftWheel->computeBoundingBox();
    leftCenter = leftWheel->worldToLocal(leftBox.getCenter());
    Box3 rightBox = rightWheel->computeBoundingBox();
    rightCenter  = rightWheel->worldToLocal(rightBox.getCenter());

    //save parent center as local coordinate
    const auto parentBox = parent->computeBoundingBox();
    parentCenter = parent->worldToLocal(parentBox.getCenter());

    update(parent->quaternion());

    {
      const auto leftCenter = parent->worldToLocal(leftCenterWorld);
      const auto rightCenter = parent->worldToLocal(rightCenterWorld);
      const auto leftToCenter = (parentCenter - leftCenter).normalized();
      const auto rightToCenter = (parentCenter - rightCenter).normalized();
      const auto midLocal = (leftCenter + rightCenter) * 0.5;

      Vector3 up(0, 0, 1);
      up.apply(parent->quaternion());
      auto crossVal = cross(leftToCenter, rightToCenter);
      float leftRight = dot(crossVal, up);

      //determine in what direction we're oriented
      float parentFront = (parentCenter - front).inclination();
      float localFront = (parentCenter - midLocal).inclination();
      bool isFront = parentFront < 0 ? localFront < 0 : localFront > 0;
      bool isRight = parentFront < 0 ? leftRight < 0 : leftRight > 0;
      pos = isRight ?
               (isFront ? WheelHinge::Position::FRONTRIGHT : WheelHinge::Position::BACKRIGHT) :
               (isFront ? WheelHinge::Position::FRONTLEFT : WheelHinge::Position::BACKLEFT);

      qDebug() << toString(pos) << leftRight << isFront << localFront << parentFront;

      dir = isFront ? 1 : -1;
    }

    switch(pos) {
      case WheelHinge::Position::BACKLEFT:
      case WheelHinge::Position::BACKRIGHT:
        dir = -1;
        break;
      case WheelHinge::Position::FRONTRIGHT:
      case WheelHinge::Position::FRONTLEFT:
        dir = 1;
        break;
    }
    if(leftWheel->position().isNull() && rightWheel->position().isNull()) {
      leftAxis = (leftCenter - leftWheel->worldToLocal(rightCenterWorld)).normalized();
      rightAxis = (rightCenter - rightWheel->worldToLocal(leftCenterWorld)).normalized();
    }
    else if(pos == Position::FRONTRIGHT || pos == Position::BACKLEFT) {
      dir = -dir;
      leftAxis = (leftWheel->worldToLocal(rightCenterWorld) - leftCenter).normalized();
      rightAxis = (rightCenter - rightWheel->worldToLocal(leftCenterWorld)).normalized();
    }
    else if(pos == Position::BACKRIGHT || pos == Position::FRONTLEFT) {
      if(pos == Position::FRONTLEFT) dir = -dir;
      leftAxis = (leftCenter - leftWheel->worldToLocal(rightCenterWorld)).normalized();
      rightAxis = (rightWheel->worldToLocal(leftCenterWorld) - rightCenter).normalized();
    }

    parent->quaternion().onChange.connect(this, &WheelHinge::update);
  }

  using Ptr = std::shared_ptr<WheelHinge>;
  static Ptr load(Object3D::Ptr object, const QJsonObject &hingeObject)
  {
    std::string name = hingeObject["name"].toString().toStdString();

    const QString parentName = hingeObject["parent"].toString();
    const auto pn = parentName.toStdString();
    Object3D::Ptr parent = pn == object->name() ? object : object->getChildByName(pn);

    const QString leftName = hingeObject["left"].toString();
    const auto an = leftName.toStdString();
    Object3D::Ptr leftWheel = an == object->name() ? object : object->getChildByName(an);

    const QString rightName = hingeObject["right"].toString();
    const auto en = rightName.toStdString();
    Object3D::Ptr rightWheel = en == object->name() ? object : object->getChildByName(en);

    const QJsonObject frontObject = hingeObject["front"].toObject();
    Vector3 front(frontObject["x"].toDouble(), frontObject["y"].toDouble(), frontObject["z"].toDouble());

    if (!leftWheel || !rightWheel) {

      qCritical() << "Hinge definition does not match model: " << (!leftWheel ? leftName : rightName)
                  << "not found";
      return nullptr;
    }

    return WheelHinge::make(name, leftWheel, rightWheel, front, parent.get());
  }

  static Ptr make(const std::string &name, Object3D::Ptr leftWheel, Object3D::Ptr rightWheel, const Vector3 &front, Object3D *parent)
  {
    return Ptr(new WheelHinge(name, leftWheel, rightWheel, front, parent));
  }
};

void Dynamics::update()
{
  auto elapsed = _timer.restart();
  if(elapsed > 0) {
    for(auto &hinge : _hinges) {
      if(!hinge->continuous) continue;

      float angle = float(M_PI * hinge->upm / 60000.0 * elapsed);
      if(hinge->angleLimit > 0 && hinge->rotatedAngle + angle > hinge->angleLimit) continue;

      hinge->rotatedAngle += angle;
      hinge->rotate(angle);
    }
  }
}

void Dynamics::fastforward(float seconds)
{
  for(auto &hinge : _hinges) {
    if(!hinge->continuous) continue;

    float angle = float(M_PI * hinge->upm / 60.0 * seconds);
    if(hinge->angleLimit > 0) {
      float theta = std::min(abs(angle), hinge->angleLimit);
      angle = angle < 0 ? -theta : theta;
    }

    hinge->rotatedAngle += angle;
    hinge->rotate(angle);
  }
}

void Dynamics::fastforward(const QString &name, float seconds)
{
  for(auto &hinge : _hinges) {
    if(hinge->name == name.toStdString()) {
      float angle = float(M_PI * hinge->upm / 60.0 * seconds);
      if(hinge->angleLimit > 0) {
        float theta = std::min(abs(angle), hinge->angleLimit);
        angle = angle < 0 ? -theta : theta;
      }

      hinge->rotatedAngle += angle;
      hinge->rotate(angle);
    }
  }
}

QStringList Dynamics::hingeNames() const
{
  QStringList names;
  for(const auto &hinge : _hinges) names.append(QString::fromStdString(hinge->name));
  return names;
}

bool Dynamics::deleteHinge(const QString &name)
{
  std::string qname = name.toStdString();
  for (auto it = _hinges.begin(); it != _hinges.end(); it++) {
    const auto &hinge = *it;
    if (hinge->name == qname) {
      _hinges.erase(it);

      emit hingesChanged();
      return true;
    }
  }
  return false;
}

void Dynamics::loadHinges(const QJsonValueRef &json, Object3D::Ptr object)
{
  resetAll();

  const QJsonArray hingeArray = json.toArray();

  for (int i = 0; i < hingeArray.size(); i++) {
    QJsonObject hingeObject = hingeArray[i].toObject();

    const auto ht = hingeObject["type"].toString();
    if (ht == "door") {
      DoorHinge::Ptr doorHinge = DoorHinge::load(object, hingeObject);
      _hinges.push_back(doorHinge);
    }
    else if (ht == "propeller") {
      PropellerHinge::Ptr propellerHinge = PropellerHinge::load(object, hingeObject);
      _hinges.push_back(propellerHinge);
    }
    else if (ht == "wheel") {
      WheelHinge::Ptr wheelHinge = WheelHinge::load(object, hingeObject);
      _hinges.push_back(wheelHinge);
      continue;
    }
    else
      throw std::invalid_argument("unknown hinge type");
  }
  emit hingesChanged();
}

bool Dynamics::load(const QString &file, ThreeQObject *object)
{
  QUrl fileUrl(file);
  QFile loadFile(fileUrl.isLocalFile() ? fileUrl.toLocalFile() : file);

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qCritical() << "Couldn't open file " << file;
    return false;
  }

  QByteArray saveData = loadFile.readAll();
  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

  const QJsonValueRef &hingeRef = loadDoc.object()["hinges"];
  if (!hingeRef.isNull()) {
    loadHinges(hingeRef, object->object());
    return true;
  }
  return false;
}

bool Dynamics::save(const QString &file)
{
  QUrl fileUrl(file);
  QFile saveFile(fileUrl.isValid() ? fileUrl.toLocalFile() : file);

  if (!saveFile.open(QIODevice::WriteOnly)) {
    qCritical("Couldn't open file.");
    return false;
  }

  QJsonArray hingeArray;

  for (const auto &hinge : hinges()) {
    QJsonObject hingeObject;
    hinge->save(hingeObject);
    hingeArray.push_back(hingeObject);
  }

  QJsonObject docObject;
  docObject["hinges"] = hingeArray;

  QJsonDocument saveDoc(docObject);
  saveFile.write(saveDoc.toJson());

  return true;
}

void Dynamics::createPropellerHinge(QString name, QVariant propeller, QVector3D back)
{
  auto *prop = propeller.value<quick::ThreeQObject *>();

  if (!prop) {
    qCritical() << "createPropellerHinge: null parameter";
    return;
  }
  Vector3 point(back.x(), back.y(), back.z());
  Object3D::Ptr p = prop->object();
  PropellerHinge::Ptr hinge = PropellerHinge::make(name.toStdString(), p, p->worldToLocal(point));

  _hinges.push_back(hinge);
  hinge->upm = 240;

  emit hingesChanged();
}

void Dynamics::createDoorHinge(QString name, QVariant door, QVariant body, QVector3D upper, QVector3D lower)
{
  auto *elem = door.value<quick::ThreeQObject *>();
  auto *bdy = body.value<quick::ThreeQObject *>();

  if (!elem || !bdy) {
    qCritical() << "createDoorHinge: null parameter";
    return;
  }

  Object3D::Ptr anchor = bdy->object();
  Object3D::Ptr element = elem->object();

  //save as local points
  Vector3 ptw1(upper.x(), upper.y(), upper.z());
  Vector3 point1 = anchor->worldToLocal(ptw1);
  Vector3 ptw2(lower.x(), lower.y(), lower.z());
  Vector3 point2 = anchor->worldToLocal(ptw2);

  DoorHinge::Ptr hinge = DoorHinge::make(name.toStdString(), anchor, element, point1, point2, float(M_PI_2 * 0.8));
  _hinges.push_back(hinge);

  emit hingesChanged();
}

void Dynamics::createWheelHinge(QString name, QVariant left, QVariant right, QVector3D front)
{
  auto *leftWheel = left.value<quick::ThreeQObject *>();
  auto *rightWheel = right.value<quick::ThreeQObject *>();

  if (!leftWheel || !rightWheel) {
    qCritical() << "createWheelHinge: null parameter";
    return;
  }
  Object3D::Ptr leftW = leftWheel->object();
  Object3D::Ptr rightW = rightWheel->object();

  //find common parent for wheels
  Object3D *parent=nullptr, *wl=leftW.get(), *wr=rightW.get();
  for(int i=0; wl->parent() && wr->parent(); i++) {
    if(wl->parent() == wr->parent()) {
      parent = wl->parent();
      break;
    }
    if(i % 2 == 0)
      wl = wl->parent();
    else
      wr = wr->parent();
  }
  if(!parent) {
    qCritical() << "createWheelHinge: cannot determine common parent";
    return;
  }
  Vector3 pfront = parent->worldToLocal(Vector3(front.x(), front.y(), front.z()));

  WheelHinge::Ptr hinge = WheelHinge::make(name.toStdString(), leftW, rightW, pfront, parent);
  _hinges.push_back(hinge);

  emit hingesChanged();
}

void Dynamics::resetAll()
{
  _hinges.clear();
  emit hingesChanged();
}

}
}
