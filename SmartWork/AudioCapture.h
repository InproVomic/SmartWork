#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QList>

class AudioCapture : public QObject
{
    Q_OBJECT

public:
    enum RecordingState {
        Stopped,
        Recording,
        Paused
    };

    explicit AudioCapture(QObject *parent = nullptr);
    ~AudioCapture();

    bool startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    RecordingState state() const { return m_state; }
    qint64 duration() const { return m_duration; }
    QString lastFilePath() const { return m_lastFilePath; }
    QAudioFormat format() const { return m_format; }
    QString getCurrentFormatString() const;
    QString getCurrentDeviceName() const;
    
    QList<QAudioDeviceInfo> availableInputDevices();
    bool setInputDevice(const QAudioDeviceInfo &device);
    void refreshDeviceList();

signals:
    void recordingStarted();
    void recordingStopped(const QString &filePath);
    void recordingError(const QString &error);
    void durationChanged(qint64 milliseconds);
    void stateChanged(RecordingState state);
    void audioLevelChanged(qreal level);

private slots:
    void onAudioReady();
    void onStateChanged(QAudio::State state);

private:
    bool initializeAudio();
    void calculateAudioLevel(const QByteArray &data);
    QString generateFilePath();
    bool ensureAudioDirectory();
    
#ifdef Q_OS_WIN
    void checkWindowsAudioDevices();
#endif

    QAudioInput *m_audioInput;
    QIODevice *m_audioDevice;
    QFile *m_outputFile;
    
    QAudioFormat m_format;
    QAudioDeviceInfo m_inputDevice;
    QList<QAudioDeviceInfo> m_availableDevices;
    
    RecordingState m_state;
    QByteArray m_buffer;
    qint64 m_duration;
    qint64 m_bytesWritten;
    QString m_lastFilePath;
    
    static const int SAMPLE_RATE = 16000;
    static const int CHANNEL_COUNT = 1;
    static const int SAMPLE_SIZE = 16;
};

#endif // AUDIOCAPTURE_H
