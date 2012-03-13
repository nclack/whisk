#include "Frame.h"
#include "Editor.h"

Frame::Frame(Editor *editor, QGraphicsItem *parent/*=0*/)
  : QGraphicsPixmapItem(parent)
  , editor_(editor)
{ setShapeMode(BoundingRectShape);
  setTransformationMode(Qt::FastTransformation); // I think this is the default, but set anyway
}

void Frame::mousePressEvent(QGraphicsSceneMouseEvent *event)
{ if(event->button()!=Qt::LeftButton)
    return;

  if(editor_)
  { editor_->traceAtAndIdentify(event->pos());
  }
}

