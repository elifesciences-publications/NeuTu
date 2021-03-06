#ifndef ZFLYEMBOOKMARKVIEW_H
#define ZFLYEMBOOKMARKVIEW_H

#include <QTableView>
#include <QString>

class ZFlyEmBookmarkListModel;
class ZFlyEmBookmark;
class QSortFilterProxyModel;

class ZFlyEmBookmarkView : public QTableView
{
  Q_OBJECT
public:
  explicit ZFlyEmBookmarkView(QWidget *parent = 0);

  ZFlyEmBookmarkListModel* getModel() const;

  void setBookmarkModel(ZFlyEmBookmarkListModel *model);
  void checkCurrentBookmark(bool checking);

  const ZFlyEmBookmark* getBookmark(const QModelIndex &viewIndex) const;

  void sort();

signals:
  void bookmarkChecked(QString key, bool checking);
  void bookmarkChecked(ZFlyEmBookmark*);
  void locatingBookmark(const ZFlyEmBookmark*);

public slots:
  void checkCurrentBookmark();
  void uncheckCurrentBookmark();

private slots:
  void processDouleClick(const QModelIndex &index);

private:
  void init();
  void createMenu();
  void connectSignalSlot();

protected:
  void contextMenuEvent(QContextMenuEvent *);
  void mousePressEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);

private:
  QMenu *m_contextMenu;
  ZFlyEmBookmarkListModel *m_bookmarkModel;
  QSortFilterProxyModel* m_proxy;
};

#endif // ZFLYEMBOOKMARKVIEW_H
