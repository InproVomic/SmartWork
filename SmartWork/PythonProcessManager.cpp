#include "PythonProcessManager.h"
#include <QProcess>
#include <QDebug>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

PythonProcessManager::PythonProcessManager(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
{
}

PythonProcessManager::~PythonProcessManager()
{
    stopProcess();
}

bool PythonProcessManager::startProcess(const QString& pythonPath, const QString& scriptPath)
{
    if (m_process && m_process->state() == QProcess::Running) {
        qDebug() << "Python process is already running";
        return true;
    }

    qDebug() << "Checking script path:" << scriptPath;
    if (!QFile::exists(scriptPath)) {
        qDebug() << "Python script not found:" << scriptPath;
        emit processError(QString("Script not found: %1").arg(scriptPath));
        return false;
    }

    qDebug() << "Python path:" << pythonPath;

    m_process = new QProcess(this);
    
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(m_process, &QProcess::started, this, &PythonProcessManager::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PythonProcessManager::onProcessFinished);
    connect(m_process, QOverload<QProcess::ProcessError>::of(&QProcess::error), this, &PythonProcessManager::onProcessError);
    
    connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        qDebug() << "Python stdout:" << m_process->readAllStandardOutput();
    });
    connect(m_process, &QProcess::readyReadStandardError, [this]() {
        qDebug() << "Python stderr:" << m_process->readAllStandardError();
    });

    qDebug() << "Starting Python process:" << pythonPath << scriptPath;
    m_process->start(pythonPath, QStringList() << scriptPath);

    if (!m_process->waitForStarted(10000)) {
        qDebug() << "Failed to start Python process:" << m_process->errorString();
        emit processError(QString("Failed to start: %1").arg(m_process->errorString()));
        return false;
    }

    qDebug() << "Python process started successfully, PID:" << m_process->processId();
    return true;
}

void PythonProcessManager::stopProcess()
{
    if (m_process) {
        if (m_process->state() == QProcess::Running) {
            qDebug() << "Stopping Python process...";
            m_process->terminate();
            if (!m_process->waitForFinished(2000)) {
                qDebug() << "Force killing Python process...";
                m_process->kill();
            }
        }
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void PythonProcessManager::onProcessStarted()
{
    qDebug() << "Python process started signal received";
    emit processStarted();
}

void PythonProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Python process finished with exit code:" << exitCode << "status:" << exitStatus;
    QString output = m_process->readAll();
    if (!output.isEmpty()) {
        qDebug() << "Python output:" << output;
    }
    emit processFinished(exitCode, exitStatus);
}

void PythonProcessManager::onProcessError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
    case QProcess::FailedToStart:
        errorString = "Failed to start";
        break;
    case QProcess::Crashed:
        errorString = "Crashed";
        break;
    case QProcess::Timedout:
        errorString = "Timed out";
        break;
    case QProcess::ReadError:
        errorString = "Read error";
        break;
    case QProcess::WriteError:
        errorString = "Write error";
        break;
    case QProcess::UnknownError:
    default:
        errorString = "Unknown error";
        break;
    }
    qDebug() << "Python process error:" << errorString << m_process->errorString();
    emit processError(errorString + ": " + m_process->errorString());
}