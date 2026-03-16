#include "LAppDelegate.hpp"		// 一定要第一个导入
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppDefine.hpp"
#include <QTimer>
#include <QMouseEvent>

#include "GLCore.h"


GLCore::GLCore(QWidget *parent)
	: QOpenGLWidget(parent)
{
	//this->setAttribute(Qt::WA_DeleteOnClose);  // 窗口关闭时自动删除对象
	this->setWindowFlags(Qt::FramelessWindowHint);  // 无边框窗口
	this->setWindowFlag(Qt::WindowStaysOnTopHint);  // 窗口置顶
	//this->setWindowFlag(Qt::Tool);  // 工具窗口，不显示在任务栏
	this->setAttribute(Qt::WA_TranslucentBackground);  // 背景透明

	_timer = std::make_unique<QTimer>(this);
	connect(_timer.get(), &QTimer::timeout, this, [=]() {
		update();
		});
	_timer->start(static_cast<int>((1.0 / 60) * 1000));  // 每秒 60 帧
}

GLCore::~GLCore()
{

}

void GLCore::initializeGL()
{
	LAppDelegate::GetInstance()->Initialize(this);
}

void GLCore::paintGL()
{
	LAppDelegate::GetInstance()->Update();
}

void GLCore::resizeGL(int w, int h)
{
	LAppDelegate::GetInstance()->Resize(w, h);
}

void GLCore::mousePressEvent(QMouseEvent* event)
{
	LAppDelegate::GetInstance()->GetView()->OnTouchesBegan(event->x(), event->y());

	if (event->button() == Qt::LeftButton)
	{
		// 记录左键按下状态和当前鼠标位置
		this->_isLeftButton = true;
		this->_currentPos = event->pos();
	}

	if (event->button() == Qt::RightButton)
	{
		this->_isRightButton = true;
	}
}

void GLCore::mouseReleaseEvent(QMouseEvent* event)
{
	LAppDelegate::GetInstance()->GetView()->OnTouchesEnded(event->x(), event->y());

	if (event->button() == Qt::LeftButton)
	{
		this->_isLeftButton = false;
	}

	if (event->button() == Qt::RightButton)
	{
		this->_isRightButton = false;
	}
}

void GLCore::mouseMoveEvent(QMouseEvent* event)
{
	LAppDelegate::GetInstance()->GetView()->OnTouchesMoved(event->x(), event->y());

	if (this->_isLeftButton)
	{
		this->move(event->pos() - this->_currentPos + this->pos());
	}
}