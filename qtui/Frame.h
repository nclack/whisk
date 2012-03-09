#pragma once
#include <QtGui>

/*
 * Design note
 * -----------
 *
 * Right now, this class doesn't really do much at all.  It's 
 * a placeholder in case more complex interaction with the 
 * image is required.
 *
 * If signals/slots are desired this (maybe) should move to a 
 * QGraphicsWidget base class with the pixmap as a subitem.
 */

class Editor;
/**
 * Shows the current image of the video.
 *
 * Handles right clicks by calling back up to the editor
 * which maybe traces a whisker.  The Editor class has
 * to have a method:
 * \code
 *    void traceAt(QPointF r);
 * \endcode 
 *
 * This is also a convenient place to configure the pixmap 
 * item.
 */
class Frame : public QGraphicsPixmapItem
{
  public:
    Frame(Editor *editor, QGraphicsItem *parent=0);

  protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);

  Editor *editor_;
};
