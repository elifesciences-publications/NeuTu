#ifndef RESOLUTIONDIALOG_H
#define RESOLUTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ResolutionDialog;
}

class ResolutionDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ResolutionDialog(QWidget *parent = 0);
  ~ResolutionDialog();

  double getXYScale() const;
  double getZScale() const;

private:
  Ui::ResolutionDialog *ui;
};

#endif // RESOLUTIONDIALOG_H
