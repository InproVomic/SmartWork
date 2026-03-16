#pragma once

#include <QWidget>
#include <QOpenGLWidget>
#include <QTimer>
#include <memory>

class GLCore : public QOpenGLWidget
{
	Q_OBJECT
public:
	GLCore(QWidget* parent = nullptr);
	~GLCore();

protected:
	std::unique_ptr<QTimer> _timer;
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	bool _isLeftButton = false;
	bool _isRightButton = false;
	QPoint _currentPos; // 当前鼠标位置


};

