#include "polylinetool.h"

#include "editor.h"
#include "scribblearea.h"

#include "strokemanager.h"
#include "layermanager.h"

#include "layervector.h"
#include "layerbitmap.h"


PolylineTool::PolylineTool( QObject *parent ) :
BaseTool( parent )
{
}

ToolType PolylineTool::type()
{
    return POLYLINE;
}

void PolylineTool::loadSettings()
{
    m_enabledProperties[WIDTH] = true;
    m_enabledProperties[BEZIER] = true;
    m_enabledProperties[ANTI_ALIASING] = true;

    QSettings settings( PENCIL2D, PENCIL2D );

    properties.width = settings.value( "polyLineWidth" ).toDouble();
    properties.feather = -1;
    properties.pressure = 0;
    properties.invisibility = OFF;
    properties.preserveAlpha = OFF;
    properties.useAA = settings.value( "brushAA").toBool();

    // First run
    if ( properties.width <= 0 )
    {
        setWidth(1.5);
    }
}

void PolylineTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;

    // Update settings
    QSettings settings( PENCIL2D, PENCIL2D );
    settings.setValue("polyLineWidth", width);
    settings.sync();
}

void PolylineTool::setFeather( const qreal feather )
{
    properties.feather = -1;
}

void PolylineTool::setAA( const bool AA )
{
    // Set current property
    properties.useAA = AA;

    // Update settings
    QSettings settings( PENCIL2D, PENCIL2D );
    settings.setValue("brushAA", AA);
    settings.sync();
}

QCursor PolylineTool::cursor() //Not working this one, any guru to fix it?
{
    if ( isAdjusting ) { // being dynamically resized
        qDebug() << "adjusting";
        return QCursor( this->circleCursors() ); // two circles cursor
    }
    return Qt::CrossCursor;
}

void PolylineTool::clear()
{
    mPoints.clear();
}

void PolylineTool::mousePressEvent( QMouseEvent *event )
{
    Layer* layer = mEditor->layers()->currentLayer();

    if ( event->button() == Qt::LeftButton )
    {
        if ( layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR )
        {
            if ( mPoints.size() == 0 )
            {
                mEditor->backup( tr( "Polyline" ) );
            }

            if ( layer->type() == Layer::VECTOR )
            {
                ( ( LayerVector * )layer )->getLastVectorImageAtFrame( mEditor->currentFrame(), 0 )->deselectAll();
                if ( mScribbleArea->makeInvisible() && !mEditor->preference()->isOn(SETTING::INVISIBLE_LINES) )
                {
                    mScribbleArea->toggleThinLines();
                }
            }
            mPoints << getCurrentPoint();
            mScribbleArea->setAllDirty();
        }
    }
}

void PolylineTool::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED( event );
}

void PolylineTool::mouseMoveEvent( QMouseEvent *event )
{
    Q_UNUSED( event );
    Layer* layer = mEditor->layers()->currentLayer();

    if ( layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR )
    {
       drawPolyline( mPoints, getCurrentPoint() );
    }
}

void PolylineTool::mouseDoubleClickEvent( QMouseEvent *event )
{
    // XXX highres position ??
    if ( BezierCurve::eLength( m_pStrokeManager->getLastPressPixel() - event->pos() ) < 2.0 )
    {
        endPolyline( mPoints );
        clear();
    }
}

bool PolylineTool::keyPressEvent( QKeyEvent *event )
{
    switch ( event->key() ) {
    case Qt::Key_Return:
        if ( mPoints.size() > 0 )
        {
            endPolyline( mPoints );
            clear();
            return true;
        }
        break;

    case Qt::Key_Escape:
        if ( mPoints.size() > 0 ) {
            cancelPolyline( );
            clear();
            return true;
        }
        break;

    default:
        return false;
    }

    return false;
}

void PolylineTool::drawPolyline(QList<QPointF> points, QPointF endPoint)
{
    if ( !mScribbleArea->areLayersSane() )
    {
        return;
    }

    if ( points.size() > 0 )
    {
        QPen pen( mEditor->color()->frontColor(),
                   properties.width,
                   Qt::SolidLine,
                   Qt::RoundCap,
                   Qt::RoundJoin );
        Layer* layer = mEditor->layers()->currentLayer();

        // Bitmap by default
        QPainterPath tempPath;
        if ( properties.bezier_state )
        {
            tempPath = BezierCurve( points ).getSimplePath();
        }
        else
        {
            tempPath = BezierCurve( points ).getStraightPath();
        }
        tempPath.lineTo( endPoint );

        // Vector otherwise
        if ( layer->type() == Layer::VECTOR )
        {
            if ( mEditor->layers()->currentLayer()->type() == Layer::VECTOR )
            {
                tempPath = mEditor->view()->mapCanvasToScreen( tempPath );
                if ( mScribbleArea->makeInvisible() == true )
                {
                    pen.setWidth( 0 );
                    pen.setStyle( Qt::DotLine );
                }
                else
                {
                    pen.setWidth(properties.width * mEditor->view()->scaling() );
                }
            }
        }

        mScribbleArea->drawPolyline(tempPath, pen, properties.useAA);
    }
}


void PolylineTool::cancelPolyline()
{
    // Clear the in-progress polyline from the bitmap buffer.
    mScribbleArea->clearBitmapBuffer();
    mScribbleArea->updateCurrentFrame();
}

void PolylineTool::endPolyline( QList<QPointF> points )
{
    if ( !mScribbleArea->areLayersSane() )
    {
        return;
    }

    Layer* layer = mEditor->layers()->currentLayer();

    if ( layer->type() == Layer::VECTOR )
    {
        BezierCurve curve = BezierCurve( points );
        if ( mScribbleArea->makeInvisible() == true )
        {
            curve.setWidth( 0 );
        }
        else
        {
            curve.setWidth( properties.width );
        }
        curve.setColourNumber( mEditor->color()->frontColorNumber() );
        curve.setVariableWidth( false );
        curve.setInvisibility( mScribbleArea->makeInvisible() );
        //curve.setSelected(true);
        ( ( LayerVector * )layer )->getLastVectorImageAtFrame( mEditor->currentFrame(), 0 )->addCurve( curve, mEditor->view()->scaling() );
    }
    if ( layer->type() == Layer::BITMAP )
    {
        drawPolyline( points, points.last() );
        BitmapImage *bitmapImage = ( ( LayerBitmap * )layer )->getLastBitmapImageAtFrame( mEditor->currentFrame(), 0 );
        bitmapImage->paste( mScribbleArea->mBufferImg );
    }
    mScribbleArea->mBufferImg->clear();
    mScribbleArea->setModified( mEditor->layers()->currentLayerIndex(), mEditor->currentFrame() );
}
