#include "zdvidsparsevolslice.h"
#include "zpainter.h"

ZDvidSparsevolSlice::ZDvidSparsevolSlice() : ZObject3dScan(), m_currentZ(-1)
{
  m_type = ZStackObject::TYPE_DVID_SPARSEVOL_SLICE;
  setHittable(false);
}

ZDvidSparsevolSlice::ZDvidSparsevolSlice(const ZDvidSparsevolSlice &obj) :
  ZObject3dScan(obj)
{
  m_currentZ = obj.m_currentZ;
  m_dvidTarget = obj.m_dvidTarget;
}

void ZDvidSparsevolSlice::setDvidTarget(const ZDvidTarget &target)
{
  m_dvidTarget = target;
  m_reader.open(target);
}

void ZDvidSparsevolSlice::update(int z)
{
  if (m_currentZ != z) {
    m_currentZ = z;
    if (z < getMinZ() || z > getMaxZ()) {
      m_reader.readBody(getLabel(), m_currentZ, this);
    }
  }
}

void ZDvidSparsevolSlice::update()
{
  m_reader.readBody(getLabel(), m_currentZ, this);
}

void ZDvidSparsevolSlice::display(
    ZPainter &painter, int slice, EDisplayStyle option) const
{
  if (slice >= 0) {
    const_cast<ZDvidSparsevolSlice&>(*this).update(painter.getZOffset() + slice);
    ZObject3dScan::display(painter, slice, option);
  }
}
