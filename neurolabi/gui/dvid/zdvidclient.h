#ifndef ZDVIDCLIENT_H
#define ZDVIDCLIENT_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QFile>
#include <QVariant>
#include <QNetworkReply>
#include <QVector>
#include <QQueue>

#include "zobject3dscan.h"
#include "zswctree.h"
#include "zprogressable.h"
#include "zdvidrequest.h"
#include "zstack.hxx"

class ZDvidBuffer;

/*!
 * \brief The class if DVID client
 */
class ZDvidClient : public QObject, ZProgressable
{
  Q_OBJECT

public:
  ZDvidClient(const QString &server, QObject *parent = NULL);

  inline void setServer(const QString &server) {
    m_serverAddress = server;
  }
  inline const QString &getServer() {
    return m_serverAddress;
  }

  inline const QString& getTmpDirectory() {
    return m_tmpDirectory;
  }

  /*!
   * \brief Send a request to DVID
   */
  bool postRequest(ZDvidRequest::EDvidRequest request, const QVariant &parameter);

  inline const ZObject3dScan& getObject() const { return m_obj; }
  inline const ZSwcTree& getSwcTree() const { return m_swcTree; }
  inline const ZStack& getImage() const { return m_image; }

  inline ZDvidBuffer* getDvidBuffer() const { return m_dvidBuffer; }

  void appendRequest(ZDvidRequest request);

  //Make ready for making new requests
  void reset();

signals:
  void objectRetrieved();
  void swcRetrieved();
  void imageRetrieved();
  void noRequestLeft();
  void requestFailed();

public slots:
  void postNextRequest();

private slots:
  bool writeObject();
  void finishRequest(QNetworkReply::NetworkError error = QNetworkReply::NoError);
  void readObject();
  void readSwc();
  void readImage();
  void cancelRequest();

private:
  void createConnection();
  inline bool isCanceling() { return m_isCanceling; }

private:
  QString m_serverAddress; //Server address
  QString m_dataPath;
  QNetworkAccessManager *m_networkManager;
  QNetworkReply *m_networkReply;
  QString m_targetDirectory;
  QString m_tmpDirectory;
  QFile *m_file;

  ZObject3dScan m_obj;
  ZSwcTree m_swcTree;
  ZStack m_image;
  QByteArray m_objectBuffer;
  QByteArray m_swcBuffer;
  QByteArray m_imageBuffer;

  QIODevice *m_uploadStream;

  ZDvidBuffer *m_dvidBuffer;
  QQueue<ZDvidRequest> m_requestQueue;
  QVariant m_currentRequestParameter;

  bool m_isCanceling;
  //int m_requestIndex;
};

#endif // ZDVIDCLIENT_H