#ifndef PYTHONPROCESSMANAGER_H
#define PYTHONPROCESSMANAGER_H

#include <QObject>
#include <QProcess>

class PythonProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit PythonProcessManager(QObject *parent = nullptr);
    ~PythonProcessManager();

    bool startProcess(const QString& pythonPath, const QString& scriptPath);
    void stopProcess();

signals:
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(const QString& error);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess* m_process;
};

#endif // PYTHONPROCESSMANAGER_H