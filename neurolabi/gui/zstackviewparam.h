#ifndef ZSTACKVIEWPARAM_H
#define ZSTACKVIEWPARAM_H

#include <QRect>
#include "neutube.h"

/*!
 * \brief The class of stack view parameter
 */
class ZStackViewParam
{
public:
  ZStackViewParam();
  ZStackViewParam(NeuTube::ECoordinateSystem coordSys);

  inline NeuTube::ECoordinateSystem getCoordinateSystem() const {
    return m_coordSys;
  }

  inline int getZ() const {
    return m_z;
  }

  inline const QRect& getViewPort() const {
    return m_viewPort;
  }

  void setZ(int z);
  void setViewPort(const QRect &rect);
  void setViewPort(int x0, int y0, int x1, int y1);

  bool operator ==(const ZStackViewParam &param) const;
  bool operator !=(const ZStackViewParam &param) const;

  bool contains(const ZStackViewParam &param) const;

  /*!
   * \brief Resize the parameter by keeping the center relatively constant
   */
  void resize(int width, int height);


private:
  int m_z;
  QRect m_viewPort;
  NeuTube::ECoordinateSystem m_coordSys;
};

#endif // ZSTACKVIEWPARAM_H
