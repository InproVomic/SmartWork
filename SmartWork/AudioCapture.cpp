#include "AudioCapture.h"
#include <QAudioInput>
#include <QDebug>
#include <QDataStream>
#include <QtMath>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "mmdevapi.lib")
#endif

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , m_audioInput(nullptr)
    , m_audioDevice(nullptr)
    , m_outputFile(nullptr)
    , m_state(Stopped)
    , m_duration(0)
    , m_bytesWritten(0)
{
    qDebug() << "=== AudioCapture Initialization ===";
    
#ifdef Q_OS_WIN
    checkWindowsAudioDevices();
#endif
    
    refreshDeviceList();
}

AudioCapture::~AudioCapture()
{
    stopRecording();
    if (m_audioInput) {
        delete m_audioInput;
        m_audioInput = nullptr;
    }
}

void AudioCapture::refreshDeviceList()
{
    m_availableDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    
    qDebug() << "=== Available Audio Input Devices ===";
    qDebug() << "Total devices found:" << m_availableDevices.size();
    
    for (int i = 0; i < m_availableDevices.size(); ++i) {
        const QAudioDeviceInfo &device = m_availableDevices[i];
        qDebug() << QString("  [%1] %2").arg(i).arg(device.deviceName());
        qDebug() << "      Supported sample rates:" << device.supportedSampleRates();
        qDebug() << "      Supported channels:" << device.supportedChannelCounts();
        qDebug() << "      Supported sample sizes:" << device.supportedSampleSizes();
    }
    
    if (m_availableDevices.isEmpty()) {
        qWarning() << "No audio input devices detected!";
    }
}

#ifdef Q_OS_WIN
void AudioCapture::checkWindowsAudioDevices()
{
    qDebug() << "=== Windows Audio Device Check ===";
    
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        qWarning() << "CoInitialize failed";
        return;
    }
    
    IMMDeviceEnumerator *pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create MMDeviceEnumerator";
        CoUninitialize();
        return;
    }
    
    IMMDevice *pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
    if (FAILED(hr)) {
        qDebug() << "No default capture device found. Error:" << hr;
        qDebug() << "This may indicate:";
        qDebug() << "  1. No microphone is connected";
        qDebug() << "  2. Microphone is disabled in Windows";
        qDebug() << "  3. Microphone access is blocked in Privacy settings";
    } else {
        LPWSTR pwszID = NULL;
        hr = pDevice->GetId(&pwszID);
        if (SUCCEEDED(hr)) {
            qDebug() << "Default capture device ID:" << QString::fromWCharArray(pwszID);
            CoTaskMemFree(pwszID);
        }
        
        IPropertyStore *pProps = NULL;
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (SUCCEEDED(hr)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
                qDebug() << "Default capture device name:" << QString::fromWCharArray(varName.pwszVal);
                PropVariantClear(&varName);
            }
            pProps->Release();
        }
        pDevice->Release();
    }
    
    pEnumerator->Release();
    CoUninitialize();
}
#endif

bool AudioCapture::initializeAudio()
{
    qDebug() << "=== initializeAudio() ===";
    
    refreshDeviceList();
    
    if (m_availableDevices.isEmpty()) {
        emit recordingError(QString::fromUtf8(
            "未检测到任何音频输入设备。\n\n"
            "请检查以下项目：\n"
            "1. 麦克风是否正确连接到电脑\n"
            "2. 打开 Windows 设置 → 系统 → 声音，检查输入设备\n"
            "3. 打开 Windows 设置 → 隐私 → 麦克风，确保允许应用访问麦克风\n"
            "4. 检查设备管理器中音频驱动是否正常"));
        return false;
    }
    
    m_inputDevice = QAudioDeviceInfo::defaultInputDevice();
    
    if (m_inputDevice.isNull() || m_inputDevice.deviceName().isEmpty()) {
        qDebug() << "Default device is invalid, using first available device";
        m_inputDevice = m_availableDevices.first();
    }
    
    qDebug() << "Using device:" << m_inputDevice.deviceName();
    
    QList<int> preferredSampleRates = {16000, 22050, 44100, 48000, 8000, 96000, 192000};
    QList<int> preferredChannelCounts = {1, 2};
    QList<int> preferredSampleSizes = {16, 8, 24, 32};
    
    bool formatFound = false;
    
    for (int sampleRate : preferredSampleRates) {
        for (int channelCount : preferredChannelCounts) {
            for (int sampleSize : preferredSampleSizes) {
                QAudioFormat testFormat;
                testFormat.setSampleRate(sampleRate);
                testFormat.setChannelCount(channelCount);
                testFormat.setSampleSize(sampleSize);
                testFormat.setCodec("audio/pcm");
                testFormat.setByteOrder(QAudioFormat::LittleEndian);
                testFormat.setSampleType(sampleSize == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
                
                if (m_inputDevice.isFormatSupported(testFormat)) {
                    m_format = testFormat;
                    formatFound = true;
                    qDebug() << "Found supported format:" << sampleRate << "Hz," << sampleSize << "bit," << channelCount << "ch";
                    break;
                }
            }
            if (formatFound) break;
        }
        if (formatFound) break;
    }
    
    if (!formatFound) {
        qDebug() << "No preferred format found, using nearest format";
        
        QAudioFormat preferredFormat;
        preferredFormat.setSampleRate(16000);
        preferredFormat.setChannelCount(1);
        preferredFormat.setSampleSize(16);
        preferredFormat.setCodec("audio/pcm");
        preferredFormat.setByteOrder(QAudioFormat::LittleEndian);
        preferredFormat.setSampleType(QAudioFormat::SignedInt);
        
        m_format = m_inputDevice.nearestFormat(preferredFormat);
        
        qDebug() << "Nearest format:" << m_format.sampleRate() << "Hz," 
                 << m_format.sampleSize() << "bit," << m_format.channelCount() << "ch";
    }
    
    if (m_format.sampleRate() <= 0 || m_format.channelCount() <= 0 || m_format.sampleSize() <= 0) {
        emit recordingError(QString::fromUtf8(
            "无法获取有效的音频格式。\n\n"
            "设备: %1\n\n"
            "请尝试：\n"
            "1. 重新插拔麦克风\n"
            "2. 重启应用程序\n"
            "3. 检查 Windows 声音设置中的录音设备").arg(m_inputDevice.deviceName()));
        return false;
    }
    
    qDebug() << "=== Selected Audio Format ===";
    qDebug() << "Sample rate:" << m_format.sampleRate() << "Hz";
    qDebug() << "Channels:" << m_format.channelCount();
    qDebug() << "Sample size:" << m_format.sampleSize() << "bits";
    qDebug() << "Codec:" << m_format.codec();
    
    if (m_audioInput) {
        delete m_audioInput;
        m_audioInput = nullptr;
    }
    
    m_audioInput = new QAudioInput(m_inputDevice, m_format, this);
    
    int bufferSize = m_format.sampleRate() * m_format.channelCount() * m_format.sampleSize() / 8 / 10;
    m_audioInput->setBufferSize(qMax(bufferSize, 3200));
    
    qDebug() << "Buffer size:" << m_audioInput->bufferSize() << "bytes";
    
    connect(m_audioInput, &QAudioInput::stateChanged, this, &AudioCapture::onStateChanged);
    connect(m_audioInput, &QAudioInput::notify, this, [this]() {
        qDebug() << "Audio notify - bytes available:" << m_audioInput->bytesReady();
    });
    
    return true;
}

QList<QAudioDeviceInfo> AudioCapture::availableInputDevices()
{
    return m_availableDevices;
}

bool AudioCapture::setInputDevice(const QAudioDeviceInfo &device)
{
    if (m_state == Recording) {
        emit recordingError(QString::fromUtf8("录制中无法切换设备"));
        return false;
    }
    
    m_inputDevice = device;
    return initializeAudio();
}

bool AudioCapture::ensureAudioDirectory()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString audioDir = appPath + "/Recordings";
    
    qDebug() << "Application path:" << appPath;
    qDebug() << "Recordings directory:" << audioDir;
    
    QDir dir(audioDir);
    if (!dir.exists()) {
        qDebug() << "Creating Recordings directory...";
        if (!dir.mkpath(".")) {
            emit recordingError(QString::fromUtf8("无法创建Recordings目录: ") + audioDir);
            return false;
        }
        qDebug() << "Recordings directory created successfully";
    }
    
    qDebug() << "Recordings directory ready:" << audioDir;
    return true;
}

QString AudioCapture::generateFilePath()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString filePath = appPath + "/Recordings/recording_" + timestamp + ".pcm";
    
    qDebug() << "Generated file path:" << filePath;
    
    return filePath;
}

bool AudioCapture::startRecording()
{
    qDebug() << "=== startRecording() ===";
    
    if (m_state == Recording) {
        qDebug() << "Already recording";
        return true;
    }
    
    refreshDeviceList();
    
    if (m_availableDevices.isEmpty()) {
        emit recordingError(QString::fromUtf8(
            "未检测到任何音频输入设备。\n\n"
            "故障排除步骤：\n\n"
            "1. 检查麦克风连接\n"
            "   - 确保麦克风已正确插入电脑的麦克风接口\n"
            "   - 如果是 USB 麦克风，尝试更换 USB 接口\n\n"
            "2. 检查 Windows 声音设置\n"
            "   - 右键点击任务栏音量图标 → 打开声音设置\n"
            "   - 在\"输入\"部分查看是否显示麦克风设备\n"
            "   - 尝试对着麦克风说话，查看音量条是否有反应\n\n"
            "3. 检查 Windows 隐私设置\n"
            "   - 打开 设置 → 隐私 → 麦克风\n"
            "   - 确保\"允许应用访问麦克风\"已开启\n"
            "   - 确保\"允许桌面应用访问麦克风\"已开启\n\n"
            "4. 检查设备管理器\n"
            "   - 右键\"此电脑\" → 管理 → 设备管理器\n"
            "   - 展开\"音频输入和输出\"\n"
            "   - 检查麦克风设备是否正常，无黄色感叹号"));
        return false;
    }
    
    if (!m_audioInput || m_format.sampleRate() <= 0) {
        if (!initializeAudio()) {
            return false;
        }
    }
    
    if (!ensureAudioDirectory()) {
        return false;
    }
    
    m_lastFilePath = generateFilePath();
    m_outputFile = new QFile(m_lastFilePath, this);
    
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        emit recordingError(QString::fromUtf8("无法创建文件: ") + m_lastFilePath);
        delete m_outputFile;
        m_outputFile = nullptr;
        return false;
    }
    
    m_buffer.clear();
    m_bytesWritten = 0;
    m_duration = 0;
    
    qDebug() << "Starting audio input...";
    qDebug() << "Device:" << m_inputDevice.deviceName();
    qDebug() << "Format:" << m_format.sampleRate() << "Hz," << m_format.sampleSize() << "bit," << m_format.channelCount() << "ch";
    
    m_audioDevice = m_audioInput->start();
    
    if (!m_audioDevice) {
        QAudio::Error err = m_audioInput->error();
        QString errorDetail;
        
        switch (err) {
            case QAudio::OpenError:
                errorDetail = QString::fromUtf8("无法打开音频设备（设备可能被其他程序占用）");
                break;
            case QAudio::IOError:
                errorDetail = QString::fromUtf8("音频设备读写错误");
                break;
            case QAudio::UnderrunError:
                errorDetail = QString::fromUtf8("音频缓冲区欠载");
                break;
            case QAudio::FatalError:
                errorDetail = QString::fromUtf8("音频设备致命错误");
                break;
            default:
                errorDetail = QString::fromUtf8("未知错误 (code: %1)").arg(err);
        }
        
        emit recordingError(QString::fromUtf8(
            "无法启动音频输入设备。\n\n"
            "设备: %1\n"
            "格式: %2 Hz, %3 bit, %4 声道\n"
            "错误: %5\n\n"
            "请尝试：\n"
            "1. 关闭其他正在使用麦克风的应用程序\n"
            "2. 重新启动应用程序\n"
            "3. 检查麦克风是否被禁用")
            .arg(m_inputDevice.deviceName())
            .arg(m_format.sampleRate())
            .arg(m_format.sampleSize())
            .arg(m_format.channelCount())
            .arg(errorDetail));
        
        m_outputFile->close();
        delete m_outputFile;
        m_outputFile = nullptr;
        return false;
    }
    
    connect(m_audioDevice, &QIODevice::readyRead, this, &AudioCapture::onAudioReady);
    
    m_state = Recording;
    emit recordingStarted();
    emit stateChanged(m_state);
    
    qDebug() << "Recording started successfully:" << m_lastFilePath;
    return true;
}

void AudioCapture::stopRecording()
{
    if (m_state == Stopped) {
        qDebug() << "stopRecording: Already stopped";
        return;
    }
    
    qDebug() << "=== stopRecording() ===";
    qDebug() << "Total bytes written:" << m_bytesWritten;
    
    if (m_audioInput) {
        m_audioInput->stop();
    }
    
    if (m_audioDevice) {
        disconnect(m_audioDevice, &QIODevice::readyRead, this, &AudioCapture::onAudioReady);
        m_audioDevice = nullptr;
    }
    
    if (m_outputFile) {
        qDebug() << "Flushing and closing file:" << m_outputFile->fileName();
        m_outputFile->flush();
        m_outputFile->close();
        
        QFileInfo fileInfo(m_outputFile->fileName());
        qDebug() << "File saved:" << fileInfo.absoluteFilePath();
        qDebug() << "File size:" << fileInfo.size() << "bytes";
        
        if (fileInfo.size() == 0) {
            qWarning() << "WARNING: File is empty! No audio data was recorded.";
        }
        
        delete m_outputFile;
        m_outputFile = nullptr;
    }
    
    m_state = Stopped;
    emit recordingStopped(m_lastFilePath);
    emit stateChanged(m_state);
    
    qDebug() << "Recording stopped. Duration:" << m_duration << "ms, Bytes:" << m_bytesWritten;
}

void AudioCapture::pauseRecording()
{
    if (m_state != Recording) {
        return;
    }
    
    if (m_audioInput) {
        m_audioInput->suspend();
    }
    
    m_state = Paused;
    emit stateChanged(m_state);
}

void AudioCapture::resumeRecording()
{
    if (m_state != Paused) {
        return;
    }
    
    if (m_audioInput) {
        m_audioInput->resume();
    }
    
    m_state = Recording;
    emit stateChanged(m_state);
}

void AudioCapture::onAudioReady()
{
    if (!m_audioDevice || !m_outputFile || m_state != Recording) {
        qDebug() << "onAudioReady: Invalid state - device:" << (m_audioDevice != nullptr) 
                 << "file:" << (m_outputFile != nullptr) << "state:" << m_state;
        return;
    }
    
    QByteArray data = m_audioDevice->readAll();
    if (data.isEmpty()) {
        qDebug() << "onAudioReady: No data available";
        return;
    }
    
    qint64 bytesWritten = m_outputFile->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "onAudioReady: Write error! Expected:" << data.size() << "Actual:" << bytesWritten;
    }
    
    m_bytesWritten += data.size();
    
    qDebug() << "onAudioReady: Read" << data.size() << "bytes, Total:" << m_bytesWritten << "bytes";
    
    qint64 bytesPerMs = (m_format.sampleRate() * m_format.channelCount() * m_format.sampleSize() / 8) / 1000;
    if (bytesPerMs > 0) {
        m_duration = m_bytesWritten / bytesPerMs;
        emit durationChanged(m_duration);
    }
    
    calculateAudioLevel(data);
}

void AudioCapture::onStateChanged(QAudio::State state)
{
    qDebug() << "Audio state changed:" << state << "Error:" << (m_audioInput ? m_audioInput->error() : -1);
    
    switch (state) {
        case QAudio::ActiveState:
            qDebug() << "Audio is now active";
            break;
        case QAudio::SuspendedState:
            qDebug() << "Audio is suspended";
            break;
        case QAudio::StoppedState:
            if (m_state == Recording) {
                if (m_audioInput && m_audioInput->error() != QAudio::NoError) {
                    QString errorStr;
                    switch (m_audioInput->error()) {
                        case QAudio::OpenError:
                            errorStr = QString::fromUtf8("无法打开音频设备");
                            break;
                        case QAudio::IOError:
                            errorStr = QString::fromUtf8("音频设备读写错误");
                            break;
                        case QAudio::UnderrunError:
                            errorStr = QString::fromUtf8("音频缓冲区欠载");
                            break;
                        case QAudio::FatalError:
                            errorStr = QString::fromUtf8("音频设备致命错误");
                            break;
                        default:
                            errorStr = QString::fromUtf8("未知音频错误");
                    }
                    emit recordingError(errorStr);
                    stopRecording();
                }
            }
            break;
        case QAudio::IdleState:
            qDebug() << "Audio is idle";
            break;
    }
}

void AudioCapture::calculateAudioLevel(const QByteArray &data)
{
    if (data.size() < 2) {
        return;
    }
    
    int sampleSize = m_format.sampleSize() / 8;
    int sampleCount = data.size() / sampleSize;
    
    if (sampleCount <= 0) {
        return;
    }
    
    qint64 sumSquares = 0;
    
    if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::SignedInt) {
        const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
        for (int i = 0; i < sampleCount; ++i) {
            qint32 sample = qAbs(samples[i]);
            sumSquares += sample * sample;
        }
    } else if (m_format.sampleSize() == 8) {
        const quint8 *samples = reinterpret_cast<const quint8*>(data.constData());
        for (int i = 0; i < sampleCount; ++i) {
            qint32 sample = qAbs(static_cast<qint32>(samples[i]) - 128);
            sumSquares += sample * sample;
        }
    } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::SignedInt) {
        const qint32 *samples = reinterpret_cast<const qint32*>(data.constData());
        for (int i = 0; i < sampleCount; ++i) {
            qint64 sample = qAbs(samples[i]);
            sumSquares += sample * sample / 65536;
        }
    }
    
    qreal rms = 0.0;
    if (sampleCount > 0) {
        rms = qSqrt(static_cast<qreal>(sumSquares) / sampleCount);
    }
    
    qreal maxVal = static_cast<qreal>(1 << (m_format.sampleSize() - 1));
    qreal level = rms / maxVal;
    emit audioLevelChanged(level);
}

QString AudioCapture::getCurrentFormatString() const
{
    QString channelStr = m_format.channelCount() == 1 ? QString::fromUtf8("单声道") : QString::fromUtf8("立体声");
    return QString::fromUtf8("%1 Hz, %2 bit, %3")
        .arg(m_format.sampleRate())
        .arg(m_format.sampleSize())
        .arg(channelStr);
}

QString AudioCapture::getCurrentDeviceName() const
{
    return m_inputDevice.deviceName();
}
