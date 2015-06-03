#ifndef ZDVIDWRITER_H
#define ZDVIDWRITER_H

#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QMap>

#include <string>
#include <vector>
#include "zobject3dscan.h"
#include "zstack.hxx"
#include "zdvidclient.h"
#include "flyem/zflyem.h"
#include "zintcuboid.h"
#include "zsparsestack.h"
#include "dvid/zdvidtarget.h"
#include "dvid/zdvidwriter.h"

class ZFlyEmNeuron;
class ZClosedCurve;
class ZIntCuboid;
class ZSwcTree;

class ZDvidWriter : public QObject
{
  Q_OBJECT
public:
  explicit ZDvidWriter(QObject *parent = 0);

  bool open(const QString &serverAddress, const QString &uuid,
            int port = -1);
  bool open(const ZDvidTarget &target);
  bool open(const QString &sourceString);

  void writeSwc(int bodyId, ZSwcTree *tree);
  void writeThumbnail(int bodyId, ZStack *stack);
  void writeThumbnail(int bodyId, Stack *stack);
  void writeAnnotation(int bodyId, const ZJsonObject &obj);
  void writeAnnotation(const ZFlyEmNeuron &neuron);
  void writeRoiCurve(const ZClosedCurve &curve, const std::string &key);
  void deleteRoiCurve(const std::string &key);
  void writeJsonString(const std::string &dataName, const std::string &key,
                       const std::string jsonString);
  void writeJson(const std::string &dataName, const std::string &key,
                 const ZJsonValue &obj);
  void writeBoundBox(const ZIntCuboid &cuboid, int z);

  //void writeSplitLabel(const ZObject3dScan &obj, int label);

  void createData(const std::string &type, const std::string &name);

  void writeBodyInfo(int bodyId, const ZJsonObject &obj);
  void writeBodyInfo(int bodyId);
  //void writeMaxBodyId(int bodyId);

  void mergeBody(const std::string &dataName, int targetId,
                 const std::vector<int> &bodyId);

  /*!
   * \brief Create a new keyvalue data in DVID.
   *
   * It does nothing if the data has already existed.
   */
  void createKeyvalue(const std::string &name);

  void deleteKey(const std::string &dataName, const std::string &key);
  void deleteKey(const QString &dataName, const QString &key);

  void deleteKey(const std::string &dataName,
                 const std::string &minKey, const std::string &maxKey);
  void deleteKey(const QString &dataName,
                 const QString &minKey, const QString &maxKey);

  void postLog(const std::string &message);
  bool lockNode(const std::string &message);
  std::string createBranch();

  void writeSplit(const std::string &dataName, const ZObject3dScan &obj,
                  uint64_t oldLabel, uint64_t label);

  void writeMergeOperation(const QMap<uint64_t, uint64_t> &bodyMap);
  /*
  void writeMergeOperation(const std::string &dataName, const std::string &key,
                           const QMap<uint64_t, uint64_t> &bodyMap);
                           */

private:
  std::string getJsonStringForCurl(const ZJsonValue &obj) const;
  void writeJson(const std::string url, const ZJsonValue &value);
  void writeJsonString(const std::string url, const std::string jsonString);

  ZJsonValue getLocMessage(const std::string &message);

private:
  QEventLoop *m_eventLoop;
  ZDvidClient *m_dvidClient;
  QTimer *m_timer;
  ZDvidTarget m_dvidTarget;
};

#endif // ZDVIDWRITER_H
