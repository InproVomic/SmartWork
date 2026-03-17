#include "GLCore.h"
#include "ChatClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Live2D Window
    GLCore live2dWindow;
    live2dWindow.show();

    // Chat Client Window (separate from Live2D)
    ChatClient chatWindow;
    chatWindow.show();

    return a.exec();
}
