#include "zflyemroidialog.h"
#include <QtConcurrentRun>
#include <QMessageBox>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMenu>

#include "neutubeconfig.h"
#include "ui_zflyemroidialog.h"
#include "dvid/zdvidreader.h"
#include "dvid/zdvidinfo.h"
#include "dvid/zdvidwriter.h"
#include "mainwindow.h"
#include "zstackframe.h"
#include "zswcgenerator.h"
#include "zjsonfactory.h"
#include "zcuboid.h"
#include "zintcuboid.h"

ZFlyEmRoiDialog::ZFlyEmRoiDialog(QWidget *parent) :
  QDialog(parent), ZProgressable(),
  ui(new Ui::ZFlyEmRoiDialog), m_project(NULL)
{
  ui->setupUi(this);

  m_dvidDlg = ZDialogFactory::makeDvidDialog(this);
  m_zDlg = ZDialogFactory::makeSpinBoxDialog(this);

  connect(ui->loadGrayScalePushButton, SIGNAL(clicked()),
          this, SLOT(loadGrayscale()));
  connect(ui->dvidServerPushButton, SIGNAL(clicked()),
          this, SLOT(setDvidTarget()));
  connect(ui->estimateRoiPushButton,
          SIGNAL(clicked()), this, SLOT(estimateRoi()));
  connect(ui->saveResultPushButton, SIGNAL(clicked()), this, SLOT(addRoi()));
  connect(ui->zSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setZ(int)));
  connect(ui->fullRoiPreviewPushButton, SIGNAL(clicked()),
          this, SLOT(previewFullRoi()));
  connect(ui->uploadResultPushButton, SIGNAL(clicked()),
          this, SLOT(uploadRoi()));

#if 0
  QSize iconSize(24, 24);
  ui->movexDecPushButton->setIconSize(iconSize);
  ui->movexIncPushButton->setIconSize(iconSize);
  ui->movexyDecPushButton->setIconSize(iconSize);
  ui->movexyIncPushButton->setIconSize(iconSize);
  ui->moveyDecPushButton->setIconSize(iconSize);
  ui->moveyIncPushButton->setIconSize(iconSize);

  ui->xDecPushButton->setIconSize(iconSize);
  ui->yDecPushButton->setIconSize(iconSize);
  ui->xyDecPushButton->setIconSize(iconSize);
  ui->xyIncPushButton->setIconSize(iconSize);
  ui->xIncPushButton->setIconSize(iconSize);
  ui->yIncPushButton->setIconSize(iconSize);

  QSize buttonSize(40, 24);
  ui->movexDecPushButton->setMaximumSize(buttonSize);
  ui->movexIncPushButton->setMaximumSize(buttonSize);
  ui->movexyDecPushButton->setMaximumSize(buttonSize);
  ui->movexyIncPushButton->setMaximumSize(buttonSize);
  ui->moveyDecPushButton->setMaximumSize(buttonSize);
  ui->moveyIncPushButton->setMaximumSize(buttonSize);

  ui->xDecPushButton->setMaximumSize(buttonSize);
  ui->yDecPushButton->setMaximumSize(buttonSize);
  ui->xyDecPushButton->setMaximumSize(buttonSize);
  ui->xyIncPushButton->setMaximumSize(buttonSize);
  ui->xIncPushButton->setMaximumSize(buttonSize);
  ui->yIncPushButton->setMaximumSize(buttonSize);
#endif

  connect(this, SIGNAL(newDocReady()), this, SLOT(newDataFrame()));
  connect(this, SIGNAL(progressFailed()), ui->progressBar, SLOT(reset()));
  connect(this, SIGNAL(progressAdvanced(double)),
          this, SLOT(advanceProgressSlot(double)));
  connect(this, SIGNAL(progressDone()), ui->progressBar, SLOT(reset()));
  connect(ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
          this, SLOT(loadProject(int)));

  if (m_project != NULL) {
    m_project->setZ(ui->zSpinBox->value());
  }
  //m_project->setDvidTarget(m_dvidDlg->getDvidTarget());

  updateWidget();

  ZQtBarProgressReporter *reporter = new ZQtBarProgressReporter;
  reporter->setProgressBar(ui->progressBar);
  setProgressReporter(reporter);
  //setProgressBar(ui->progressBar);
  ui->progressBar->hide();

#ifndef _DEBUG_
  ui->testPushButton->hide();
#endif

  createMenu();
}

ZFlyEmRoiDialog::~ZFlyEmRoiDialog()
{
  m_projectList.clear();
  delete ui;
}

void ZFlyEmRoiDialog::createMenu()
{
  m_nextMenu = new QMenu(this);

  QAction *actionNext1 = new QAction("1", this);
  m_nextMenu->addAction(actionNext1);
  //ui->nextPushButton->setMenu(m_nextMenu);

  m_mainMenu = new QMenu(this);
  ui->menuPushButton->setMenu(m_mainMenu);
  QAction *exportAction = new QAction("Export", this);
  m_mainMenu->addAction(exportAction);
  connect(exportAction, SIGNAL(triggered()), this, SLOT(exportResult()));
}

void ZFlyEmRoiDialog::clear()
{
  ui->projectComboBox->clear();
  foreach (ZFlyEmRoiProject *proj, m_projectList) {
    delete proj;
  }
  m_projectList.clear();
  m_project = NULL;
}

void ZFlyEmRoiDialog::updateWidget()
{
  ui->pushButton->setEnabled(m_dvidTarget.isValid());
  if (m_project != NULL) {
    QString text = QString("<p>DVID: %1</p>"
                           "<p>Z Range: [%2, %3]"
                           "<p>Opened Slice: Z = %4; ROI: %5</p>").
        arg(m_project->getDvidTarget().getName().c_str()).
        arg(m_project->getDvidInfo().getMinZ()).
        arg(m_project->getDvidInfo().getMaxZ()).
        arg(m_project->getDataZ()).arg(m_project->hasOpenedRoi());
    ui->infoWidget->setText(text);
    ui->zSpinBox->setValue(m_project->getDataZ());

    ui->loadGrayScalePushButton->setEnabled(m_project->getDvidTarget().isValid());
    ui->searchPushButton->setEnabled(m_project->getDvidTarget().isValid());
    ui->estimateRoiPushButton->setEnabled(m_project->hasDataFrame());
  } else {
    ui->infoWidget->setText("");
    ui->loadGrayScalePushButton->setEnabled(false);
    ui->searchPushButton->setEnabled(false);
    ui->estimateRoiPushButton->setEnabled(false);
  }
}

ZFlyEmRoiProject* ZFlyEmRoiDialog::getProject(size_t index)
{
  if (index >= (size_t) m_projectList.size()) {
    return NULL;
  }

  return m_projectList[index];
}

bool ZFlyEmRoiDialog::appendProject(ZFlyEmRoiProject *project)
{
  if (project != NULL) {
    if (!project->getName().empty()) {
      bool isValidProject = true;
      foreach (ZFlyEmRoiProject *proj, m_projectList) {
        if (proj->getName() == project->getName()) {
          isValidProject = false;
        }
      }
      if (isValidProject) {
        project->downloadAllRoi();
        m_projectList.append(project);
        ui->projectComboBox->addItem(project->getName().c_str());
        return true;
      }
    }
  }

  return false;
}

void ZFlyEmRoiDialog::downloadAllProject()
{
  ZDvidReader reader;
  if (reader.open(getDvidTarget())) {
    clear();
    QByteArray value = reader.readKeyValue("roi_curve", "projects");
    ZJsonArray array;
    array.decode(value.constData());
    for (size_t i = 0; i < array.size(); ++i) {
      std::string name(ZJsonParser::stringValue(array.at(i)));
      if (!name.empty()) {
        ZFlyEmRoiProject *project = newProject(name);
        appendProject(project);
      }
    }
    m_project = getProject(0);
    if (m_project != NULL) {
      setZ(ui->zSpinBox->value());
    }
  }
}


void ZFlyEmRoiDialog::setDvidTarget()
{
  if (m_dvidDlg->exec()) {
    m_dvidTarget = m_dvidDlg->getDvidTarget();
    dump(QString("Dvid server set to %1").
         arg(m_dvidTarget.getSourceString().c_str()));

    //load all projects
    downloadAllProject();
    if (!m_projectList.isEmpty()) {
      dump(QString("%1 projects found. The current project is set to \"%2\"").
           arg(m_projectList.size()).arg(m_project->getName().c_str()), true);
    } else {
      dump("No project exists. You need to new a project to proceed.", true);
    }

    updateWidget();

    /*
    if (m_project != NULL) {
      m_project->setDvidTarget(m_dvidTarget);
      updateWidget();
    }
    */
  }
}

void ZFlyEmRoiDialog::loadPartialGrayscaleFunc(
    int x0, int x1, int y0, int y1, int z)
{
  if (m_project == NULL) {
    return;
  }

  //advance(0.1);
  emit progressAdvanced(0.1);
  ZDvidReader reader;
  if (z >= 0 && reader.open(m_project->getDvidTarget())) {
    if (m_project->getRoi(z) == NULL) {
      m_project->downloadRoi(z);
    }

    QString infoString = reader.readInfo("grayscale");

    qDebug() << infoString;

    ZDvidInfo dvidInfo;
    dvidInfo.setFromJsonString(infoString.toStdString());

    ZStack *stack = NULL;
    stack = reader.readGrayScale(x0, y0, z,
                                 x1 - x0 + 1,
                                 y1 - y0 + 1, 1);

    if (stack != NULL) {
      //advance(0.5);
      emit progressAdvanced(0.5);
      m_docReader.clear();
      m_docReader.setStack(stack);

      ZSwcTree *tree = m_project->getRoiSwc(z);
      if (tree != NULL) {
        m_docReader.addObject(
              tree, NeuTube::Documentable_SWC, ZDocPlayer::ROLE_ROI);
      }
      emit newDocReady();
    } else {
      emit progressFailed();
    }
  } else {
    emit progressFailed();
  }
}

void ZFlyEmRoiDialog::loadGrayscaleFunc(int z)
{
  if (m_project == NULL) {
    return;
  }

  //advance(0.1);
  emit progressAdvanced(0.1);
  ZDvidReader reader;
  if (z >= 0 && reader.open(m_project->getDvidTarget())) {
    if (m_project->getRoi(z) == NULL) {
      m_project->downloadRoi(z);
    }

    QString infoString = reader.readInfo("grayscale");

    qDebug() << infoString;

    ZDvidInfo dvidInfo;
    dvidInfo.setFromJsonString(infoString.toStdString());

    //int z = m_zDlg->getValue();
    //m_project->setZ(z);

    ZIntCuboid boundBox = reader.readBoundBox(z);

    ZStack *stack = NULL;
    if (!boundBox.isEmpty()) {
      stack = reader.readGrayScale(boundBox.getFirstCorner().getX(),
                                   boundBox.getFirstCorner().getY(),
                                   z, boundBox.getWidth(),
                                   boundBox.getHeight(), 1);
    } else {
      stack = reader.readGrayScale(
            dvidInfo.getStartCoordinates().getX(),
            dvidInfo.getStartCoordinates().getY(),
            z, dvidInfo.getStackSize()[0],
          dvidInfo.getStackSize()[1], 1);
      if (stack != NULL) {
        boundBox = ZFlyEmRoiProject::estimateBoundBox(*stack);
        if (!boundBox.isEmpty()) {
          stack->crop(boundBox);
        }
        ZDvidWriter writer;
        if (writer.open(m_project->getDvidTarget())) {
          writer.writeBoundBox(boundBox, z);
        }
      }
    }

    if (stack != NULL) {
      //advance(0.5);
      emit progressAdvanced(0.5);
      m_docReader.clear();
      m_docReader.setStack(stack);

      ZSwcTree *tree = m_project->getRoiSwc(z);
      if (tree != NULL) {
        m_docReader.addObject(
              tree, NeuTube::Documentable_SWC, ZDocPlayer::ROLE_ROI);
      }
      emit newDocReady();
    } else {
      emit progressFailed();
    }
  } else {
    emit progressFailed();
  }
}

void ZFlyEmRoiDialog::newDataFrame()
{
  ZStackFrame *frame = getMainWindow()->createStackFrame(
        m_docReader, NeuTube::Document::FLYEM_ROI);
  frame->document()->setStackBackground(NeuTube::IMAGE_BACKGROUND_BRIGHT);
  setDataFrame(frame);

  getMainWindow()->addStackFrame(frame);
  getMainWindow()->presentStackFrame(frame);
  updateWidget();
  endProgress();
}

void ZFlyEmRoiDialog::loadGrayscale(int z)
{
  if (m_project == NULL || z < 0) {
    return;
  }

  bool loading = true;
  if (m_project->isRoiSaved() == false) {
    int ret = QMessageBox::warning(
          this, "Unsaved Result",
          "The current ROI is not saved. Do you want to continue?",
          QMessageBox::Yes | QMessageBox::No);
    loading = (ret == QMessageBox::Yes);
  }

  if (loading) {
    startProgress();
    QtConcurrent::run(
          this, &ZFlyEmRoiDialog::loadGrayscaleFunc, z);
  }
}

void ZFlyEmRoiDialog::loadGrayscale(const ZIntCuboid &box)
{
  int z = box.getFirstCorner().getZ();
  if (m_project == NULL || z < 0) {
    return;
  }

  bool loading = true;
  if (m_project->isRoiSaved() == false) {
    int ret = QMessageBox::warning(
          this, "Unsaved Result",
          "The current ROI is not saved. Do you want to continue?",
          QMessageBox::Yes | QMessageBox::No);
    loading = (ret == QMessageBox::Yes);
  }

  if (loading) {
    startProgress();
    QtConcurrent::run(
          this, &ZFlyEmRoiDialog::loadPartialGrayscaleFunc,
          box.getFirstCorner().getX(), box.getLastCorner().getX(),
          box.getFirstCorner().getY(), box.getLastCorner().getY(),
          box.getFirstCorner().getZ());
  }
}

void ZFlyEmRoiDialog::loadGrayscale()
{
  if (m_project == NULL) {
    return;
  }

  loadGrayscale(m_project->getCurrentZ());
}

void ZFlyEmRoiDialog::setDataFrame(ZStackFrame *frame)
{
  if (m_project == NULL) {
    return;
  }

  connect(frame, SIGNAL(destroyed()), this, SLOT(shallowClearDataFrame()));
  m_project->setDataFrame(frame);
}

MainWindow* ZFlyEmRoiDialog::getMainWindow()
{
  return dynamic_cast<MainWindow*>(this->parentWidget());
}

void ZFlyEmRoiDialog::shallowClearDataFrame()
{
  if (m_project == NULL) {
    return;
  }

  m_project->shallowClearDataFrame();
  updateWidget();
}

void ZFlyEmRoiDialog::addRoi()
{
  if (m_project == NULL) {
    return;
  }

  if (!m_project->addRoi()) {
    dump("The result cannot be saved because the ROI is invalid.");
  } else {
    dump("ROI saved successfully.");
    updateWidget();
  }
}

void ZFlyEmRoiDialog::dump(const QString &str, bool appending)
{
  if (appending) {
    ui->outputWidget->append(str);
  } else {
    ui->outputWidget->setText(str);
  }
}

void ZFlyEmRoiDialog::setZ(int z)
{
  if (m_project == NULL) {
    return;
  }

  m_project->setZ(z);

  ui->nextSlicePushButton->setEnabled(getNextZ() >= 0);
  ui->prevSlicePushButton->setEnabled(getPrevZ() >= 0);
  //updateWidget();
}

void ZFlyEmRoiDialog::previewFullRoi()
{
  ZSwcTree *tree = m_project->getAllRoiSwc();

  if (tree != NULL) {
    ZStackFrame *frame = new ZStackFrame();
    frame->document()->addSwcTree(tree);

    frame->open3DWindow(this);
    delete frame;
  } else {
    dump("No ROI saved");
  }
}

void ZFlyEmRoiDialog::uploadRoi()
{
  if (m_project == NULL) {
    return;
  }

  int count = m_project->uploadRoi();
  dump(QString("%1 ROI curves uploaded").arg(count));
}

void ZFlyEmRoiDialog::estimateRoi()
{
  if (m_project == NULL) {
    return;
  }

  m_project->estimateRoi();
}

void ZFlyEmRoiDialog::on_searchPushButton_clicked()
{
  int z = m_project->findSliceToCreateRoi(ui->zSpinBox->value());
  if (z >= 0) {
    ui->zSpinBox->setValue(z);
    loadGrayscale(z);
  }
}


void ZFlyEmRoiDialog::on_testPushButton_clicked()
{
  ZObject3dScan obj = m_project->getRoiObject();
  obj.downsampleMax(2, 2, 2);

  ZSwcTree *tree = ZSwcGenerator::createSurfaceSwc(obj);
  if (tree != NULL) {
    ZStackFrame *frame = new ZStackFrame();
    frame->document()->addSwcTree(tree);

    frame->open3DWindow(this);
    delete frame;
  }


  //obj.save(GET_DATA_DIR + "/test.sobj");

  //dump(QString("%1 saved.").arg((GET_DATA_DIR + "/test.sobj").c_str()));
}

#define ROI_SCALE 1.01

void ZFlyEmRoiDialog::on_xIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->scaleRoiSwc(ROI_SCALE, 1.0);
}

void ZFlyEmRoiDialog::on_xDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->scaleRoiSwc(1. / ROI_SCALE, 1.0);
}


void ZFlyEmRoiDialog::on_yDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->scaleRoiSwc(1.0, 1. / ROI_SCALE);
}

void ZFlyEmRoiDialog::on_yIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->scaleRoiSwc(1.0, ROI_SCALE);
}

void ZFlyEmRoiDialog::on_rotateLeftPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->rotateRoiSwc(-0.1);
}

void ZFlyEmRoiDialog::on_rotateRightPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->rotateRoiSwc(0.1);
}

void ZFlyEmRoiDialog::on_xyDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }

  m_project->scaleRoiSwc(1. / ROI_SCALE, 1. / ROI_SCALE);
}

void ZFlyEmRoiDialog::on_xyIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->scaleRoiSwc(ROI_SCALE, ROI_SCALE);
}

#define MOVE_STEP 5.0

void ZFlyEmRoiDialog::on_movexyDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(-MOVE_STEP, -MOVE_STEP);
}

void ZFlyEmRoiDialog::on_movexyIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(MOVE_STEP, MOVE_STEP);
}

void ZFlyEmRoiDialog::on_movexDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(-MOVE_STEP, 0.0);
}

void ZFlyEmRoiDialog::on_movexIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(MOVE_STEP, 0.0);
}

void ZFlyEmRoiDialog::on_moveyDecPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(0.0, -MOVE_STEP);
}

void ZFlyEmRoiDialog::on_moveyIncPushButton_clicked()
{
  if (m_project == NULL) {
    return;
  }
  m_project->translateRoiSwc(0.0, MOVE_STEP);
}

void ZFlyEmRoiDialog::advanceProgressSlot(double p)
{
  advanceProgress(p);
}

void ZFlyEmRoiDialog::closeEvent(QCloseEvent *event)
{
  if (m_project == NULL) {
    return;
  }

  if (!m_project->isRoiSaved()) {
    int answer = QMessageBox::warning(
          this, "Unsaved Results",
          "The current ROI has not been saved. "
          "Do you want to close the project now?",
          QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::No) {
      event->ignore();
    }
  }
  if (event->isAccepted()) {
    if (!m_project->isAllRoiCurveUploaded()) {
      int answer = QMessageBox::warning(
            this, "Unsaved Results",
            "Some ROIs seem not been uploaded into DVID. "
            "Do you want to close the project now?",
            QMessageBox::Yes | QMessageBox::No);
      if (answer == QMessageBox::No) {
        event->ignore();
      }
    }
  }
  if (event->isAccepted()) {
    m_project->closeDataFrame();
  }
}

void ZFlyEmRoiDialog::loadProject(int index)
{
  if (m_project != NULL) { //will be modified for better switching
    m_project->closeDataFrame();
  }
  m_project = getProject(index);
  updateWidget();
}

bool ZFlyEmRoiDialog::isValidName(const std::string &name) const
{
  bool isValid = false;
  if (!name.empty()) {
    isValid = true;
    foreach (ZFlyEmRoiProject *proj, m_projectList) {
      if (proj->getName() == name) {
        isValid = false;
      }
    }
  }

  return isValid;
}

ZFlyEmRoiProject* ZFlyEmRoiDialog::newProject(const std::string &name)
{
  ZFlyEmRoiProject *project = NULL;
  if (isValidName(name)) {
    project = new ZFlyEmRoiProject(name, this);
    project->setDvidTarget(getDvidTarget());
  }

  return project;
}

void ZFlyEmRoiDialog::on_pushButton_clicked() //new project
{
  bool ok;
  QString text = QInputDialog::getText(this, tr("New ROI Project"),
                                       tr("Project name:"), QLineEdit::Normal,
                                       "", &ok);
  if (ok && !text.isEmpty()) {
    ZFlyEmRoiProject *project = newProject(text.toStdString());
    if (project != NULL) {
      if (appendProject(project)) {
        if (m_projectList.size() == 1) {
          loadProject(0);
        }
        uploadProjectList();
        dump(QString("Project \"%1\" is created.").
             arg(project->getName().c_str()));
      }
    }
    updateWidget();
  }
}

void ZFlyEmRoiDialog::uploadProjectList()
{
  ZDvidWriter writer;
  if (writer.open(getDvidTarget())) {
    ZJsonArray array;
    foreach (ZFlyEmRoiProject *proj, m_projectList) {
      array.append(proj->getName());
    }

    writer.writeJsonString("roi_curve", "projects", array.dumpString(0));
  }
}

void ZFlyEmRoiDialog::on_estimateVolumePushButton_clicked()
{
  if (m_project != NULL) {
    double volume = m_project->estimateRoiVolume('u');
    dump(QString("ROI Volume: ~%1 um^3").arg(volume));
  }
}

void ZFlyEmRoiDialog::exportResult()
{
  if (m_project != NULL) {
    ZObject3dScan obj = m_project->getRoiObject();

    ZObject3dScan blockObj = m_project->getDvidInfo().getBlockIndex(obj);

    blockObj.save(GET_DATA_DIR + "/test.sobj");

    ZJsonArray array = ZJsonFactory::makeJsonArray(
          blockObj, ZJsonFactory::OBJECT_SPARSE);
    //ZJsonObject jsonObj;
    //jsonObj.setEntry("data", array);
    array.dump(GET_DATA_DIR + "/test.json");
  }
}

void ZFlyEmRoiDialog::on_exportPushButton_clicked()
{
  exportResult();
}

int ZFlyEmRoiDialog::getNextZ() const
{
  int z = -1;
  if (m_project != NULL) {
    z = m_project->getDataZ() + ui->stepSpinBox->value();
    if (z >= m_project->getDvidInfo().getMaxZ()) {
      z = -1;
    }
  }

  return z;
}

int ZFlyEmRoiDialog::getPrevZ() const
{
  int z = -1;
  if (m_project != NULL) {
    z = m_project->getDataZ() - ui->stepSpinBox->value();
  }

  return z;
}

void ZFlyEmRoiDialog::on_nextSlicePushButton_clicked()
{
  loadGrayscale(getNextZ());
}

void ZFlyEmRoiDialog::on_prevSlicePushButton_clicked()
{
  loadGrayscale(getPrevZ());
}

void ZFlyEmRoiDialog::quickLoad(int z)
{
  if (z >= 0  && m_project != NULL) {
    const ZClosedCurve *curve0 = m_project->getRoi(z);
    ZClosedCurve curve;
    if (curve0 == NULL) {
      curve = m_project->estimateRoi(z);
    } else {
      curve = *curve0;
    }

    int margin = 100;
    ZCuboid cuboid = curve.getBoundBox();
    ZIntCuboid boundBox = cuboid.toIntCuboid();
    boundBox.setFirstZ(z);
    boundBox.expandX(margin);
    boundBox.expandY(margin);
    loadGrayscale(boundBox);
  }
}
void ZFlyEmRoiDialog::on_quickPrevPushButton_clicked()
{
  int z = getPrevZ();
  quickLoad(z);
}

void ZFlyEmRoiDialog::on_quickNextPushButton_3_clicked()
{
  int z = getNextZ();
  quickLoad(z);
}