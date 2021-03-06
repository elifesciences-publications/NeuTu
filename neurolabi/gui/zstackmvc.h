#ifndef ZSTACKMVC_H
#define ZSTACKMVC_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QLayout>

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

#include "zsharedpointer.h"
//#include "zwidgetmessage.h"

class ZStackDoc;
class ZStackView;
class ZStackPresenter;
class ZStackViewParam;
class ZProgressSignal;
class ZWidgetMessage;
class QMainWindow;

/*!
 * \brief The MVC class for stack operation
 *
 * This class encloses stack data, stack view and the data presenter. It is a
 * variant of ZStackFrame, with the flexibity of not using the frame.
 */
class ZStackMvc : public QWidget
{
  Q_OBJECT
public:
  explicit ZStackMvc(QWidget *parent = 0);

  static ZStackMvc* Make(QWidget *parent, ZSharedPointer<ZStackDoc> doc);

  void attachDocument(ZStackDoc *doc);
  void attachDocument(ZSharedPointer<ZStackDoc> doc);
  void detachDocument();

  ZSharedPointer<ZStackDoc> getDocument() const {
    return m_doc;
  }

  inline ZStackPresenter* getPresenter() const {
    return m_presenter;
  }

  inline ZStackView* getView() const {
    return m_view;
  }

  /*!
   * \brief Get the global geometry of the view window.
   */
  QRect getViewGeometry() const;

  void connectSignalSlot();
  void disconnectAll();

  ZProgressSignal* getProgressSignal() const {
    return m_progressSignal;
  }

  QMainWindow* getMainWindow() const;

  virtual void processViewChangeCustom(const ZStackViewParam &/*viewParam*/) {}

protected: // Events
  virtual void keyPressEvent(QKeyEvent *event);

signals:
  void stackChanged();
  void objectChanged();
  void objectSelectionChanged();
  void messageGenerated(const ZWidgetMessage&);

public slots:
  void processViewChange(const ZStackViewParam &viewParam);
  void processViewChange();

  void dump(const QString &msg);

protected:
  static void BaseConstruct(ZStackMvc *frame, ZSharedPointer<ZStackDoc> doc);
  virtual void customInit();
  virtual void createPresenter();
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dropEvent(QDropEvent *event);
//  virtual void focusInEvent(QFocusEvent * event);
//  virtual void focusOutEvent(QFocusEvent * event);
//  virtual void changeEvent(QEvent * event);

private:
  void createView();
  void dropDocument(ZSharedPointer<ZStackDoc> doc);
  void updateDocument();
  void construct(ZSharedPointer<ZStackDoc> doc);

protected:
  ZSharedPointer<ZStackDoc> m_doc;
  ZStackPresenter *m_presenter;
  ZStackView *m_view;
  QLayout *m_layout;
  ZProgressSignal *m_progressSignal;
};

#endif // ZSTACKMVC_H
