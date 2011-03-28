//$ HEADER_NEW_FILE $
/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   CameraView.h
 *  @brief  QLabel to capture mouse and wheel events. This widget send move and zoom signals 
 *			
 */

#ifndef incl_CameraModule_CameraView_h
#define incl_CameraModule_CameraView_h

#include <QLabel>
#include <QPointF>
#include <QTime>

#include "ui_cameras.h"

class QMouseEvent;

namespace Camera
{   
    class CameraView;

    class CameraWidget: public QWidget,  public Ui::Cameras
    {
        Q_OBJECT
    public:
        /*! Constructor.
         * \param parent Parent Widget. Default 0
         */
        CameraWidget(QWidget* widget=0);

        QLabel* GetRenderer() { return reinterpret_cast<QLabel*>(renderer);};

    signals:
        void closeSignal(CameraWidget*);
        void checkBoxStateChanged(int);

    public slots:
        void SetWindowTitle(const QString &name);
        void ParentVisibilityChanged(bool visible);

    private:
        CameraView *renderer;
    };

    class CameraView: public QLabel
    {
        Q_OBJECT
    public:
        /*! Constructor.
         * \param parent Parent Widget. Default 0
         */
        CameraView(QWidget* widget=0);

    protected:
        //! Captures mouse press events
        void mousePressEvent ( QMouseEvent * e);
        
        //! Captures mouse release events
        void mouseReleaseEvent ( QMouseEvent * e);
        
        //! Captures mouse move events and emits move signal
        void mouseMoveEvent ( QMouseEvent * e);
        
        //! Captures wheel events
        void wheelEvent ( QWheelEvent * e ); 
        
    signals:
        //! Movement signal. [-1,1]
        void Move(qreal x, qreal y);

        //! Zoom signal
        void Zoom(qreal delta, Qt::KeyboardModifiers modifiers);

    private:
        //! Boolean to manage mouse left button
        bool left_mousebutton_pressed_;

        //! Last position to make movement signal
        QPointF last_pos_;

        //! Timer to update emit signal
        QTime update_timer_;
    };
}

#endif