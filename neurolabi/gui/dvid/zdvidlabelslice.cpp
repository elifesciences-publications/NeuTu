#include "zdvidlabelslice.h"

#include <QColor>
#include <QRect>

#include "zarray.h"
#include "dvid/zdvidreader.h"
#include "zobject3dfactory.h"
#include "flyem/zflyembodymerger.h"
#include "zimage.h"
#include "zpainter.h"
#include "neutubeconfig.h"

ZDvidLabelSlice::ZDvidLabelSlice()
{
  init(512, 512);
}

ZDvidLabelSlice::ZDvidLabelSlice(int maxWidth, int maxHeight)
{
  init(maxWidth, maxHeight);
}

ZDvidLabelSlice::~ZDvidLabelSlice()
{
  delete m_paintBuffer;
  delete m_labelArray;
}

void ZDvidLabelSlice::init(int maxWidth, int maxHeight)
{
  setTarget(ZStackObject::TARGET_OBJECT_CANVAS);
  m_type = ZStackObject::TYPE_DVID_LABEL_SLICE;
  m_objColorSheme.setColorScheme(ZColorScheme::CONV_RANDOM_COLOR);
  m_hitLabel = -1;
  m_bodyMerger = NULL;
  setZOrder(0);

  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;

  m_paintBuffer = new ZImage(m_maxWidth, m_maxHeight, QImage::Format_ARGB32);
  m_labelArray = NULL;
  m_selectionFrozen = false;
}

ZSTACKOBJECT_DEFINE_CLASS_NAME(ZDvidLabelSlice)

void ZDvidLabelSlice::display(
    ZPainter &painter, int slice, EDisplayStyle option) const
{
  if (isVisible()) {
    if (m_currentViewParam.getViewPort().width() > m_paintBuffer->width() ||
        m_currentViewParam.getViewPort().height() > m_paintBuffer->height()) {
      for (ZObject3dScanArray::const_iterator iter = m_objArray.begin();
           iter != m_objArray.end(); ++iter) {
        ZObject3dScan &obj = const_cast<ZObject3dScan&>(*iter);

        if (m_selectedOriginal.count(obj.getLabel()) > 0) {
          obj.setSelected(true);
        } else {
          obj.setSelected(false);
        }

        obj.display(painter, slice, option);
      }
    } else {
      m_paintBuffer->clear();
      m_paintBuffer->setOffset(-m_currentViewParam.getViewPort().x(),
                               -m_currentViewParam.getViewPort().y());

      for (ZObject3dScanArray::const_iterator iter = m_objArray.begin();
           iter != m_objArray.end(); ++iter) {
        ZObject3dScan &obj = const_cast<ZObject3dScan&>(*iter);
        //if (m_selectedSet.count(obj.getLabel()) > 0) {
        if (m_selectedOriginal.count(obj.getLabel()) > 0) {
          obj.setSelected(true);
        } else {
          obj.setSelected(false);
        }

        //      obj.display(painter, slice, option);

        if (!obj.isSelected()) {
          m_paintBuffer->setData(obj);
        } else {
          m_paintBuffer->setData(obj, QColor(255, 255, 255, 164));
        }
      }

      //    painter.save();
      //    painter.setOpacity(0.5);

      painter.drawImage(m_currentViewParam.getViewPort().x(),
                        m_currentViewParam.getViewPort().y(),
                        *m_paintBuffer);
    }

//    painter.restore();

#ifdef _DEBUG_2
//      m_paintBuffer->save((GET_TEST_DATA_DIR + "/test.tif").c_str());
#endif
  }
}

void ZDvidLabelSlice::update()
{
  if (m_objArray.empty()) {
    forceUpdate(m_currentViewParam);
  }
}


void ZDvidLabelSlice::forceUpdate()
{
  forceUpdate(m_currentViewParam);
}

void ZDvidLabelSlice::setDvidTarget(const ZDvidTarget &target)
{
  m_dvidTarget = target;
  m_reader.open(target);
}

void ZDvidLabelSlice::forceUpdate(const ZStackViewParam &viewParam)
{
  m_objArray.clear();
  if (isVisible()) {
    int yStep = 1;

//    ZDvidReader reader;
//    if (reader.open(getDvidTarget())) {
      QRect viewPort = viewParam.getViewPort();

      std::cout << "Deleting " << m_labelArray << std::endl;

      delete m_labelArray;
      m_labelArray = m_reader.readLabels64(
            getDvidTarget().getLabelBlockName(),
            viewPort.left(), viewPort.top(), viewParam.getZ(),
            viewPort.width(), viewPort.height(), 1);

      if (m_labelArray != NULL) {
        ZObject3dFactory::MakeObject3dScanArray(*m_labelArray, yStep, &m_objArray, true);

        m_objArray.translate(viewPort.left(), viewPort.top(),
                             viewParam.getZ());
        /*
        if (m_bodyMerger != NULL) {
          updateLabel(*m_bodyMerger);
        }
        */
        assignColorMap();

//        delete labelArray;
      }
    }
//  }
}

void ZDvidLabelSlice::update(int z)
{
  ZStackViewParam viewParam = m_currentViewParam;
  viewParam.setZ(z);

  update(viewParam);
}

void ZDvidLabelSlice::updateFullView(const ZStackViewParam &viewParam)
{
  forceUpdate(viewParam);

  m_currentViewParam = viewParam;
}

void ZDvidLabelSlice::update(const ZStackViewParam &viewParam)
{
  ZStackViewParam newViewParam = viewParam;
  int area = viewParam.getViewPort().width() * viewParam.getViewPort().height();
//  const int maxWidth = 512;
//  const int maxHeight = 512;
  if (area > m_maxWidth * m_maxHeight) {
    newViewParam.resize(m_maxWidth, m_maxHeight);
  }

  if (!m_currentViewParam.contains(newViewParam)) {
    forceUpdate(newViewParam);

    m_currentViewParam = newViewParam;
  }
}

QColor ZDvidLabelSlice::getColor(
    uint64_t label, NeuTube::EBodyLabelType labelType) const
{
  return getColor((int64_t) label, labelType);
}

QColor ZDvidLabelSlice::getColor(
    int64_t label, NeuTube::EBodyLabelType labelType) const
{
  QColor color;
  if (hasCustomColorMap()) {
    color = getCustomColor(label);
//    if (color.alpha() != 0) {
      color.setAlpha(64);
//    }
  } else {
    color = m_objColorSheme.getColor(
          abs((int) getMappedLabel((uint64_t) label, labelType)));
    color.setAlpha(64);
  }

  return color;
}

void ZDvidLabelSlice::setCustomColorMap(
    const ZSharedPointer<ZFlyEmBodyColorScheme> &colorMap)
{
  m_customColorScheme = colorMap;
  assignColorMap();
}

bool ZDvidLabelSlice::hasCustomColorMap() const
{
  return m_customColorScheme.get() != NULL;
}

void ZDvidLabelSlice::removeCustomColorMap()
{
  m_customColorScheme.reset();
  assignColorMap();
}

void ZDvidLabelSlice::assignColorMap()
{
  for (ZObject3dScanArray::iterator iter = m_objArray.begin();
       iter != m_objArray.end(); ++iter) {
    ZObject3dScan &obj = *iter;
    //obj.setColor(getColor(obj.getLabel()));
    obj.setColor(getColor(obj.getLabel(), NeuTube::BODY_LABEL_ORIGINAL));
  }
}

bool ZDvidLabelSlice::hit(double x, double y, double z)
{
  for (ZObject3dScanArray::iterator iter = m_objArray.begin();
       iter != m_objArray.end(); ++iter) {
    ZObject3dScan &obj = *iter;
    if (obj.hit(x, y, z)) {
      //m_hitLabel = obj.getLabel();
      m_hitLabel = getMappedLabel(obj);
      return true;
    }
  }

  return false;
}

void ZDvidLabelSlice::selectHit(bool appending)
{
  if (m_hitLabel > 0) {
    if (!appending) {
      clearSelection();
    }

    addSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
//    addSelection(m_hitLabel);
//    m_selectedOriginal.insert(m_hitLabel);
  }
}

void ZDvidLabelSlice::setSelection(std::set<uint64_t> &selected,
                                   NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    m_selectedOriginal = selected;
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(selected.begin(), selected.end());
      m_selectedOriginal.insert(
            selectedOriginal.begin(), selectedOriginal.end());
    } else {
      m_selectedOriginal = selected;
    }
    break;
  }
}

void ZDvidLabelSlice::addSelection(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    m_selectedOriginal.insert(bodyId);
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      m_selectedOriginal.insert(
            selectedOriginal.begin(), selectedOriginal.end());
    } else {
      m_selectedOriginal.insert(bodyId);
    }
    break;
  }

//  m_selectedSet.insert(getMappedLabel(bodyId, labelType));
}

void ZDvidLabelSlice::xorSelection(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    if (m_selectedOriginal.count(bodyId) > 0) {
      m_selectedOriginal.erase(bodyId);
    } else {
      m_selectedOriginal.insert(bodyId);
    }
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      xorSelectionGroup(selectedOriginal.begin(), selectedOriginal.end(),
                        NeuTube::BODY_LABEL_ORIGINAL);
    } else {
      xorSelection(bodyId, NeuTube::BODY_LABEL_ORIGINAL);
    }
  }
}

void ZDvidLabelSlice::deselectAll()
{
  m_selectedOriginal.clear();
}

bool ZDvidLabelSlice::isBodySelected(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType) const
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    return m_selectedOriginal.count(bodyId) > 0;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      for (QSet<uint64_t>::const_iterator iter = selectedOriginal.begin();
           iter != selectedOriginal.end(); ++iter) {
        if (m_selectedOriginal.count(*iter) > 0) {
          return true;
        }
      }
    } else {
      return m_selectedOriginal.count(bodyId) > 0;
    }
    break;
  }

  return false;
}

void ZDvidLabelSlice::toggleHitSelection(bool appending)
{
  bool hasSelected = isBodySelected(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
  xorSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);

  if (!appending) {
    clearSelection();

    if (!hasSelected) {
      addSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
    }
  }

  //xorSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);

  /*
  bool hasSelected = false;
  if (m_selectedSet.count(m_hitLabel) > 0) {
    hasSelected = true;
    m_selectedSet.erase(m_hitLabel);
  }

  if (!appending) {
    clearSelection();
  }

  if (!hasSelected) {
    selectHit(true);
  }
  */
}

void ZDvidLabelSlice::clearSelection()
{
  m_selectedOriginal.clear();
}

/*
void ZDvidLabelSlice::updateLabel(const ZFlyEmBodyMerger &merger)
{
  for (ZObject3dScanArray::iterator iter = m_objArray.begin();
       iter != m_objArray.end(); ++iter) {
    ZObject3dScan &obj = *iter;
    obj.setLabel(merger.getFinalLabel(obj.getLabel()));
  }
}
*/

void ZDvidLabelSlice::updateLabelColor()
{
  /*
  if (m_bodyMerger != NULL) {
    updateLabel(*m_bodyMerger);
  }
  */
  assignColorMap();
}

void ZDvidLabelSlice::setBodyMerger(ZFlyEmBodyMerger *bodyMerger)
{
  m_bodyMerger = bodyMerger;
}

void ZDvidLabelSlice::setMaxSize(int maxWidth, int maxHeight)
{
  if (m_maxWidth != maxWidth || m_maxHeight != maxHeight) {
    m_maxWidth = maxWidth;
    m_maxHeight = maxHeight;
    m_currentViewParam.resize(m_maxWidth, m_maxHeight);
    m_objArray.clear();
    delete m_paintBuffer;
    m_paintBuffer = new ZImage(m_maxWidth, m_maxHeight, QImage::Format_ARGB32);

    update();
  }
}

std::set<uint64_t> ZDvidLabelSlice::getOriginalLabelSet(
    uint64_t mappedLabel) const
{
  std::set<uint64_t> labelSet;
  if (m_bodyMerger == NULL) {
    labelSet.insert(mappedLabel);
  } else {
    const QSet<uint64_t> &sourceSet = m_bodyMerger->getOriginalLabelSet(mappedLabel);
    labelSet.insert(sourceSet.begin(), sourceSet.end());
  }

  return labelSet;
}

uint64_t ZDvidLabelSlice::getMappedLabel(const ZObject3dScan &obj) const
{
  return getMappedLabel(obj.getLabel(), NeuTube::BODY_LABEL_ORIGINAL);
}

uint64_t ZDvidLabelSlice::getMappedLabel(
    uint64_t label, NeuTube::EBodyLabelType labelType) const
{
  if (labelType == NeuTube::BODY_LABEL_ORIGINAL) {
    if (m_bodyMerger != NULL) {
      return m_bodyMerger->getFinalLabel(label);
    }
  }

  return label;
}

const ZStackViewParam& ZDvidLabelSlice::getViewParam() const
{
  return m_currentViewParam;
}

std::set<uint64_t> ZDvidLabelSlice::getSelected(
    NeuTube::EBodyLabelType labelType) const
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    return getSelectedOriginal();
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      return m_bodyMerger->getFinalLabel(getSelectedOriginal());
    } else {
      return getSelectedOriginal();
    }
    break;
  }

  return std::set<uint64_t>();
}

void ZDvidLabelSlice::mapSelection()
{
  m_selectedOriginal = getSelected(NeuTube::BODY_LABEL_MAPPED);
}

void ZDvidLabelSlice::recordSelection()
{
  m_prevSelectedOriginal = m_selectedOriginal;
}

void ZDvidLabelSlice::processSelection()
{
  m_selector.reset(m_selectedOriginal, m_prevSelectedOriginal);

#if 0
  for (std::set<uint64_t>::const_iterator iter = m_selectedOriginal.begin();
       iter != m_selectedOriginal.end(); ++iter) {
    if (m_prevSelectedOriginal.count(*iter) == 0) {
      m_selector.selectObject(*iter);
    }
  }

  for (std::set<uint64_t>::const_iterator iter = m_prevSelectedOriginal.begin();
       iter != m_prevSelectedOriginal.end(); ++iter) {
    if (m_selectedOriginal.count(*iter) == 0) {
      m_selector.deselectObject(*iter);
    }
  }
#endif
}

QColor ZDvidLabelSlice::getCustomColor(uint64_t label) const
{
  QColor color(0, 0, 0, 0);

  if (m_customColorScheme.get() != NULL) {
    color = m_customColorScheme->getBodyColor(label);
  }

  return color;
}
