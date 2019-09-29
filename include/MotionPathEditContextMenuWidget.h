//
//  MotionPathEditContextMenuWidget.h
//  MotionPath
//
//  Created by Daniele Federico on 24/01/15.
//
//

#ifndef __MotionPath__MotionPathEditContextMenuWidget__
#define __MotionPath__MotionPathEditContextMenuWidget__

#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>
#include <QtCore/QPointer>
#include <maya/MIntArray.h>

/* TO GENERATE THE MOC FILE ON A MAC
/Applications/Autodesk/maya2015/Maya.app/Contents/bin/moc -I "/Applications/Autodesk/maya2015/devkit/include/:/Applications/Autodesk/maya2015/devkit/include/qt-include" MotionPathEditContextMenuWidget.h -o MotionPathEditContextMenuWidgetMoc.cpp
*/

class ContextMenuWidget : public QWidget
{
	Q_OBJECT
    
public:
	ContextMenuWidget(QWidget *parent = 0);
	~ContextMenuWidget();
    
    public slots:
        void menuAction(QAction *action);
    
protected:
	bool eventFilter(QObject *o, QEvent *e);
    
    
private:
    
	void refreshSelection(const QPoint point);
    
    bool frame, keyframe, curve;
    int selectedCurveId;
    MIntArray selectedKeys;
    double frameTime;
    
	QWidget *m_parent;
	QPointer<QMenu> m_menu;
};

#endif /* defined(__MotionPath__MotionPathEditContextMenuWidget__) */
