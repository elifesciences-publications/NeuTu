#ifndef ZJSONFACTORY_H
#define ZJSONFACTORY_H

#if defined(_QT_GUI_USED_)
#include <QMap>
#endif

#include "tz_stdint.h"

class ZJsonArray;
class ZJsonObject;
class ZObject3dScan;

class ZJsonFactory
{
public:
  ZJsonFactory();

  enum EObjectForm {
    OBJECT_DENSE, OBJECT_SPARSE
  };

  static ZJsonArray MakeJsonArray(const ZObject3dScan &obj,
                                  EObjectForm form = OBJECT_SPARSE);

#if defined(_QT_GUI_USED_)
  static ZJsonArray MakeJsonArray(const QMap<uint64_t, uint64_t> &map);
#endif
};

#endif // ZJSONFACTORY_H
