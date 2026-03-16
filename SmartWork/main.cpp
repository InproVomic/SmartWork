#include "GLCore.h"
#include "ChatClient.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <exception>

int main(int argc, char *argv[])
{
    try {
        QApplication a(argc, argv);

        qDebug() << "Application starting...";

        GLCore live2dWindow;
        live2dWindow.show();
        qDebug() << "GLCore window shown";

        ChatClient chatWindow;
        chatWindow.show();
        qDebug() << "ChatClient window shown";

        qDebug() << "Calling initializePythonProcess...";
        chatWindow.initializePythonProcess();
        qDebug() << "initializePythonProcess called";

        return a.exec();
    } catch (const std::exception& e) {
        qDebug() << "Exception:" << e.what();
        QMessageBox::critical(nullptr, "Error", QString("Exception: %1").arg(e.what()));
        return 1;
    } catch (...) {
        qDebug() << "Unknown exception";
        QMessageBox::critical(nullptr, "Error", "Unknown exception occurred");
        return 1;
    }
}
