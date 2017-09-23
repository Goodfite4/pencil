/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2017 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>

#include "layer.h"
#include "layervector.h"
#include "layerbitmap.h"
#include "layermanager.h"
#include "colormanager.h"
#include "strokemanager.h"

#include "pencilsettings.h"
#include "editor.h"
#include "scribblearea.h"

#include "buckettool.h"

BucketTool::BucketTool( QObject *parent ) :
StrokeTool( parent )
{
}


ToolType BucketTool::type()
{
    return BUCKET;
}

void BucketTool::loadSettings()
{
    properties.width = 4;
    properties.feather = 10;
    properties.inpolLevel = 0;
    properties.useAA = -1;
    properties.tolerance = 10;

    m_enabledProperties[TOLERANCE] = true;
}

QCursor BucketTool::cursor()
{
    if( mEditor->preference()->isOn( SETTING::TOOL_CURSOR ) ) {
        QPixmap pixmap( ":icons/bucketTool.png" );
        QPainter painter( &pixmap );
        painter.end();

        return QCursor( pixmap, 4, 20 );
    } else {
        return Qt::CrossCursor;
    }
}

void BucketTool::setTolerance(const int tolerance)
{
    // Set current property
    properties.tolerance = tolerance;

    // Update settings
    QSettings settings( PENCIL2D, PENCIL2D );
    settings.setValue("tolerance", tolerance );
    settings.sync();
}


void BucketTool::mousePressEvent( QMouseEvent *event )
{
    if( event->button() == Qt::LeftButton ) {
        mEditor->backup( typeName() );
        mScribbleArea->setAllDirty();
    }

    startStroke();
}

/**
 * @brief BucketTool::getCurve
 * @param layer
 * This function gets the most recent vector curve
 */
void BucketTool::getCurve(Layer* layer)
{
    vectorImage = ( ( LayerVector * )layer )->getLastVectorImageAtFrame( mEditor->currentFrame(), 0 );
    mScribbleArea->mClosestCurves = vectorImage->getCurvesCloseTo( getCurrentPoint(),
                                                                   mScribbleArea->selectionTolerance / mEditor->view()->scaling() );
    QList<int> closestCurve = mScribbleArea->mClosestCurves;

    if ( closestCurve.size() > 0 ) {
        if ( !vectorImage->isSelected( closestCurve ) ) {
            mVectorPath = vectorImage->getStrokedPath();
            qDebug() << "stroke found";
        }
    }
}

void BucketTool::mouseReleaseEvent( QMouseEvent *event )
{
    Layer* layer = mEditor->layers()->currentLayer();
    if ( layer == NULL ) { return; }

    if ( event->button() == Qt::LeftButton ) {
        if ( layer->type() == Layer::BITMAP ) {
            paintBitmap(layer);
        }
        else if( layer->type() == Layer::VECTOR ) {
            paintVector(event, layer);
        }
    }
    endStroke();
}

void BucketTool::mouseMoveEvent( QMouseEvent *event )
{
    Layer* layer = mEditor->layers()->currentLayer();
    getCurve( layer );
    if ( layer->type() == Layer::BITMAP) {
        Q_UNUSED( event );
    }
    else if(layer->type() == Layer::VECTOR ) {
        if( event->buttons() & Qt::LeftButton ) {
            drawStroke();
            qDebug() << "DrawStroke" << event->pos() ;
        }
    }

    Q_UNUSED( event );
}

void BucketTool::paintBitmap(Layer* layer)
{
    Layer *targetLayer = layer; // by default
    int layerNumber = mEditor->layers()->currentLayerIndex(); // by default

    BitmapImage *targetImage = ( ( LayerBitmap * )targetLayer )->getLastBitmapImageAtFrame( mEditor->currentFrame(), 0 );

    QPoint point = getLastPoint().toPoint();

    QRect cameraRect = mScribbleArea->getCameraRect().toRect();

    BitmapImage::floodFill( targetImage,
                            cameraRect,
                            point,
                            Qt::transparent,
                            qPremultiply( mEditor->color()->frontColor().rgba() ),
                            properties.tolerance * 2.55 );

    mScribbleArea->setModified( layerNumber, mEditor->currentFrame() );
    mScribbleArea->setAllDirty();
}

void BucketTool::paintVector(QMouseEvent *event, Layer* layer)
{
    mScribbleArea->clearBitmapBuffer();
    VectorImage *vectorImage = ( ( LayerVector * )layer )->getLastVectorImageAtFrame( mEditor->currentFrame(), 0 );

    // TODO: does this work? what does removeArea do?
    if( event->modifiers() == Qt::AltModifier ) {
        vectorImage->removeArea( getLastPoint() );
    } else {
            QList<QPointF> path;

            for (int i = 0; i < mVectorPath.elementCount(); i++) {
                path.append(mVectorPath.elementAt(i));
            }

            // TODO: only fill when selected
            if (vectorImage->isSelected(mScribbleArea->mClosestCurves)) {
                vectorImage->fillPath(path, mEditor->color()->frontColorNumber(), mScribbleArea->selectionTolerance / mEditor->view()->scaling());
            }
//        } else {
//            vectorImage->fillPath( path,
//                               mEditor->color()->frontColorNumber(),
//                               10.0 / mEditor->view()->scaling() );
//        }
          applyChanges();
    }
    mScribbleArea->setModified( mEditor->layers()->currentLayerIndex(), mEditor->currentFrame() );
    mScribbleArea->setAllDirty();
}

void BucketTool::applyChanges()
{
    mScribbleArea->applyTransformedSelection();
}

void BucketTool::drawStroke()
{
    StrokeTool::drawStroke();

    if (properties.inpolLevel != m_pStrokeManager->getInpolLevel()) {
        m_pStrokeManager->setInpolLevel(properties.inpolLevel);
    }

    QList<QPointF> p = m_pStrokeManager->interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();


    if( layer->type() == Layer::VECTOR ) {
        mCurrentWidth = 10;
        int rad = qRound( ( mCurrentWidth / 2 + 2 ) * mEditor->view()->scaling() );

        QColor pathColor = qPremultiply(mEditor->color()->frontColor().rgba());
//        pathColor.setAlpha(255);

        QPen pen( pathColor,
                  mCurrentWidth * mEditor->view()->scaling(),
                  Qt::SolidLine,
                  Qt::RoundCap,
                  Qt::RoundJoin );

        if( p.size() == 4 ) {
            QPainterPath path( p[ 0 ] );
            path.cubicTo( p[ 1 ],
                          p[ 2 ],
                          p[ 3 ] );
            mScribbleArea->drawPath( path, pen, Qt::NoBrush, QPainter::CompositionMode_Source );
            mScribbleArea->refreshVector( path.boundingRect().toRect(), rad );
        }
    }
}
