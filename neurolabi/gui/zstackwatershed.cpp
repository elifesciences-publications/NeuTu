#include "zstackwatershed.h"
#include "neutubeconfig.h"
#include "tz_cuboid_i.h"
#include "tz_math.h"

ZStackWatershed::ZStackWatershed() : m_floodingZero(false)
{
  Cuboid_I_Set_S(&m_range, 0, 0, 0, 0, 0, 0);
}

ZStackWatershed::~ZStackWatershed()
{

}

Stack_Watershed_Workspace* ZStackWatershed::createWorkspace(const Stack *stack)
{
  Stack_Watershed_Workspace *ws = Make_Stack_Watershed_Workspace(stack);
  ws->conn = 6;
  if (C_Stack::kind(stack) == GREY) {
    ws->start_level = 255;
  } else {
    ws->start_level = 65535;
  }
  Stack *mask = C_Stack::make(GREY, C_Stack::width(stack),
                              C_Stack::height(stack), C_Stack::depth(stack));
  ws->mask = mask;
  C_Stack::setZero(mask);
  size_t voxelNumber = C_Stack::voxelNumber(stack);
  for (size_t i = 0; i < voxelNumber; ++i) {
    //Need modifcation to handle 16bit data
    if (stack->array[i] == 0) {
      mask->array[i] = STACK_WATERSHED_BARRIER;
    }
  }

  return ws;
}

void ZStackWatershed::addSeed(Stack_Watershed_Workspace *ws,
                              const ZPoint &offset,
                              const std::vector<ZStack*> &seedMask)
{
  if (ws != NULL) {
    for (std::vector<ZStack*>::const_iterator iter = seedMask.begin();
         iter != seedMask.end(); ++iter) {
      ZStack *seed = *iter;
      Stack *block = seed->c_stack();
      int x0 = iround(seed->getOffset().x() - offset.x());
      int y0 = iround(seed->getOffset().y() - offset.y());
      int z0 = iround(seed->getOffset().z() - offset.z());

      C_Stack::setBlockValue(ws->mask, block, x0, y0, z0, 0);
    }
  }
}

void ZStackWatershed::setRange(int x0, int y0, int z0, int x1, int y1, int z1)
{
  m_range.cb[0] = x0;
  m_range.cb[1] = y0;
  m_range.cb[2] = z0;
  m_range.ce[0] = x1;
  m_range.ce[1] = y1;
  m_range.ce[2] = z1;
}

ZStack *ZStackWatershed::run(
    const ZStack *stack, const std::vector<ZStack *> &seedMask)
{
  ZStack *result = NULL;
  const Stack *source = stack->c_stack();

  if (stack != NULL) {
    Cuboid_I stackBox;
    stack->getBoundBox(&stackBox);

    Cuboid_I box = m_range;
    for (int i = 0; i < 3; ++i) {
      if (m_range.ce[i] < m_range.cb[i]) {
        box.ce[i] = stackBox.ce[i];
      }
    }

    Cuboid_I_Intersect(&stackBox, &box, &box);

    if (Cuboid_I_Is_Valid(&box)) {


      /*
    int x0 = imax2(0, m_range.cb[0] - iround(stack->getOffset().x()));
    int y0 = imax2(0, m_range.cb[1] - iround(stack->getOffset().y()));
    int z0 = imax2(0, m_range.cb[2] - iround(stack->getOffset().z()));

    int width = Cuboid_I_Width(&m_range);
    int height = Cuboid_I_Height(&m_range);
    int depth = Cuboid_I_Depth(&m_range);

    if (m_range.ce[0] < m_range.cb[0] ||
        Cuboid_I_Width(&m_range) >= stack->width()) {
      width = stack->width();
    }
    if (m_range.ce[1] < m_range.cb[1] ||
        Cuboid_I_Height(&m_range) >= stack->height()) {
      height = stack->height();
    }
    if (m_range.ce[2] < m_range.cb[2] ||
        Cuboid_I_Depth(&m_range) >= stack->depth()) {
      depth = stack->depth();
    }
    */

      source = C_Stack::crop(stack->c_stack(), box, NULL);

      Stack_Watershed_Workspace *ws = createWorkspace(source);
      addSeed(ws, stack->getOffset(), seedMask);

#ifdef _DEBUG_2
      C_Stack::write(GET_DATA_DIR + "/test.tif", ws->mask);
#endif

      Stack *out = Stack_Watershed(source, ws);
      result = new ZStack;
      result->consumeData(out);
      result->setOffset(
            stack->getOffset() + ZPoint(box.cb[0], box.cb[1], box.cb[2]));

      if (source != stack->c_stack()) {
        C_Stack::kill(const_cast<Stack*>(source));
      }
      Kill_Stack_Watershed_Workspace(ws);
    }
  }

  return result;
}

ZStack* ZStackWatershed::run(const Stack *stack,
                             const std::vector<ZStack *> &seedMask)
{
  ZStack *result = NULL;
  const Stack *source = stack;

  if (stack != NULL) {
    Cuboid_I box = m_range;
    if (m_range.ce[0] < 0 || m_range.ce[0] >= C_Stack::width(stack)) {
      //extend the size to the last corner
      box.ce[0] = C_Stack::width(stack) - 1;
    }
    if (m_range.ce[1] < 0 || m_range.ce[1] >= C_Stack::height(stack)) {
      //extend the size to the last corner
      box.ce[1] = C_Stack::height(stack) - 1;
    }
    if (m_range.ce[2] < 0 || m_range.ce[2] >= C_Stack::depth(stack)) {
      //extend the size to the last corner
      box.ce[2] = C_Stack::depth(stack) - 1;
    }

    Stack_Watershed_Workspace *ws = createWorkspace(source);
    addSeed(ws, ZPoint(0, 0, 0), seedMask);

#ifdef _DEBUG_2
    C_Stack::write(GET_DATA_DIR + "/test.tif", ws->mask);
#endif

    Stack *out = Stack_Watershed(source, ws);
    result = new ZStack;
    result->consumeData(out);
    result->setOffset(box.cb[0], box.cb[1], box.cb[2]);

    if (source != stack) {
      C_Stack::kill(const_cast<Stack*>(source));
    }
    Kill_Stack_Watershed_Workspace(ws);
  }

  return result;
}