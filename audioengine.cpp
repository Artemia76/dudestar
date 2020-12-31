/*
    Copyright (C) 2019-2021 Doug McLain

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "audioengine.h"
#include <QDebug>
#include <QtEndian>
#include <QtMath>
#include <samplerate.h>

//AudioEngine::AudioEngine(QObject *parent) : QObject(parent)
AudioEngine::AudioEngine(QString in, QString out) :
    m_outputdevice(out),
    m_inputdevice(in)
{

}

AudioEngine::~AudioEngine()
{
    //m_indev->disconnect();
    //m_in->stop();
    //m_outdev->disconnect();
    //m_out->stop();
    //delete m_in;
    //delete m_out;
}

QStringList AudioEngine::discover_audio_devices(uint8_t d)
{
    QStringList list;
    QAudio::Mode m = (d) ? QAudio::AudioOutput :  QAudio::AudioInput;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(m);

    for (QList<QAudioDeviceInfo>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
        //fprintf(stderr, "Playback device name = %s\n", (*it).deviceName().toStdString().c_str());fflush(stderr);
        list.append((*it).deviceName());
    }
    return list;
}

///
/// \brief AudioEngine::init
///
void AudioEngine::init()
{
    /*
    QAudioFormat format;
    QAudioFormat tempformat;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);*/

    // Get List of available audio output on system
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    if(devices.size() == 0){
        qWarning() << tr("No audio playback hardware found");
    }
    else{
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        for (auto device : devices) {
#ifdef QT_DEBUG
            /*qDebug() << "Playback device name = " << device.deviceName();
            qDebug() << device.supportedByteOrders();
            qDebug() << device.supportedChannelCounts();
            qDebug() << device.supportedCodecs();
            qDebug() << device.supportedSampleRates();
            qDebug() << device.supportedSampleSizes();
            qDebug() << device.supportedSampleTypes();*/
#endif
            if(device.deviceName() == m_outputdevice){
                info = device;
            }
        }
        m_format_out = info.preferredFormat();
#ifdef QT_DEBUG
        qDebug()<< tr("Using playback device ") << info.deviceName();
        qDebug() << m_format_out.sampleRate();
        qDebug() << m_format_out.channelCount();
        qDebug() << m_format_out.sampleSize();
#endif
        m_out = new QAudioOutput(info, m_format_out, this);
        m_out->setBufferSize(4000);
        m_outdev = m_out->start();
    }

    //get List of availaible audio input devices on system
    devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    if(devices.size() == 0){
        qWarning() << tr("No audio recording hardware found");
    }
    else{
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
        for (auto device : devices) {
#ifdef QT_DEBUG
            /*qDebug() << "Capture device name = " << device.deviceName();
            qDebug() << device.supportedByteOrders();
            qDebug() << device.supportedChannelCounts();
            qDebug() << device.supportedCodecs();
            qDebug() << device.supportedSampleRates();
            qDebug() << device.supportedSampleSizes();
            qDebug() << device.supportedSampleTypes();*/
#endif
            if(device.deviceName() == m_inputdevice){
                info = device;
            }
        }
        m_format_in = info.preferredFormat();
#ifdef QT_DEBUG
        qDebug() << tr("Using recording device ") << info.deviceName();
        qDebug() << m_format_in.sampleRate();
        qDebug() << m_format_in.channelCount();
        qDebug() << m_format_in.sampleSize();
#endif
        m_in = new QAudioInput(info, m_format_in, this);
    }
}

void AudioEngine::set_output_volume(qreal v)
{
#ifdef QT_DEBUG
    qDebug() << "set_output_volume() v == " << v;
#endif
    m_out->setVolume(v);
}

void AudioEngine::set_input_volume(qreal v)
{
#ifdef QT_DEBUG
    qDebug() << "set_input_volume() v == " << v;
#endif
    m_in->setVolume(v);
}

void AudioEngine::start_capture()
{
    m_audioinq.clear();
    m_indev = m_in->start();
    connect(m_indev, SIGNAL(readyRead()), SLOT(input_data_received()));
}

void AudioEngine::stop_capture()
{
    m_indev->disconnect();
    m_in->stop();
}

void AudioEngine::input_data_received()
{
    QByteArray data;
    float* insample;
    qint64 len = m_in->bytesReady();
    int sampleSize = m_format_in.sampleSize();
    int channelBytes = sampleSize / 8;
    int channelCount = m_format_in.channelCount();
    if (len > 0){
        data.resize(len);
        m_indev->read(data.data(), len);
        unsigned char *ptr = reinterpret_cast<unsigned char *>(data.data());
        int step=1; // Step increment for each sample
        step = channelCount; // Get real number of channel
        if (channelCount>2) channelCount=2; // we limit channels to stereo
        switch (sampleSize){
            case 8:
                // Nothing
                break;
            case 16:
                step *=2;
                break;
            case 24:
                step *=4;//Even in 24 bit, data is stored in 32
                break;
            case 32:
                step *=4;
                break;

        }
        insample = new float[len/step];
        float* inptr = insample;
        for(int i = 0; i < len; i += step){
            float sample[2]= {0.0,0.0}; // Store sample as double to be independant of byte format
            for (int j = 0 ; j<channelCount;j++)
            {
                switch (sampleSize){
                    case 8:
                    {
                        if (m_format_in.sampleType() == QAudioFormat::UnSignedInt)
                        {
                            const quint8 value = static_cast<quint8>(data[i]);
                            sample[j] = ((static_cast<double>(value) / static_cast<double>(0xFF))*2.0)-1.0;
                        }
                        else
                        {
                            const qint8 value = static_cast<qint8>(data[i]);
                            sample[j] = static_cast<double>(value) / static_cast<double>(0x7F);
                        }
                        break;
                    }
                    case 16:
                    {
                        if (m_format_in.sampleType() == QAudioFormat::UnSignedInt)
                        {
                            quint16 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<quint16>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<quint16>(ptr);
                            }

                            sample[j] = ((static_cast<double>(value) / static_cast<double>(0xFFFF))*2.0)-1.0;
                        }
                        else
                        {
                            qint16 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<qint16>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<qint16>(ptr);
                            }
                            sample[j] = static_cast<double>(value) / static_cast<double>(0x7FFF);
                        }
                        break;
                    }
                    case 24:
                    {
                        if (m_format_in.sampleType() == QAudioFormat::UnSignedInt)
                        {
                            quint32 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<quint32>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<quint32>(ptr);
                            }

                            sample[j] = ((static_cast<double>(value) / static_cast<double>(0xFFFFFF))*2.0)-1.0;
                        }
                        else
                        {
                            qint32 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<qint32>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<qint32>(ptr);
                            }
                            sample[j] = static_cast<double>(value) / static_cast<double>(0x7FFFFF);
                        }
                        break;
                    }
                    case 32:
                    {
                        if (m_format_in.sampleType() == QAudioFormat::UnSignedInt)
                        {
                            quint32 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<quint32>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<quint32>(ptr);
                            }

                            sample[j] = ((static_cast<double>(value) / static_cast<double>(0xFFFFFFFF))*2.0)-1.0;
                        }
                        else
                        {
                            qint32 value;
                            if (m_format_in.byteOrder() == QAudioFormat::LittleEndian)
                            {
                                value = qFromLittleEndian<qint32>(ptr);
                            }
                            else
                            {
                                value = qFromBigEndian<qint32>(ptr);
                            }
                            sample[j] = static_cast<double>(value) / static_cast<double>(0x7FFFFFFF);
                        }
                        break;
                    }
                }
                ptr += channelBytes;
            }
            if (channelCount>1)
            {
                *inptr = (sample[0]+sample[1])/2.0; // Mixing both channels to mono
            }
            else
            {
                *inptr = sample[0];
            }
            inptr++;
        }
        // Now resample buffer to store it at 8Khz in m_audioinq

        SRC_DATA dsp_param;
        dsp_param.data_in= insample;
        dsp_param.src_ratio = 8000.0 / static_cast<double>(m_format_in.sampleRate());
        int out_size = qCeil((len/step)*dsp_param.src_ratio);
        float* outsample = new float[out_size];
        dsp_param.data_out=outsample;
        dsp_param.input_frames = len/step;
        dsp_param.output_frames = out_size;
        src_simple(&dsp_param,SRC_SINC_FASTEST,1);
        for (int i=0; i<out_size; i++)
        {
            m_audioinq.enqueue(static_cast<qint16>(outsample[i] * static_cast<qint16>(0x7FFF)));
        }
        delete[] outsample;
        delete[] insample;
    }
}

void AudioEngine::write(int16_t *pcm, size_t s)
{
    int sampleSize = m_format_out.sampleSize();
    int channelBytes = sampleSize / 8;
    int channelCount = m_format_out.channelCount();
    const int sampleBytes = channelCount * channelBytes;
    m_maxlevel = 0;
    SRC_DATA dsp_param;

    //Convert codec audio output to float normalized array
    float* insample = new float[s];
    for (size_t i=0; i<s ; i++)
    {
        insample[i]=static_cast<double>(pcm[i])/ static_cast<double>(0x7FFF);
    }

    //resampling audio to samplerate for output device
    dsp_param.src_ratio= qCeil(static_cast<double>(m_format_out.sampleRate()) / 8000.0);
    dsp_param.data_in =insample;
    size_t out_size = qCeil(s*dsp_param.src_ratio);
    float* outsample = new float[out_size];
    dsp_param.data_out=outsample;
    dsp_param.input_frames =s;
    dsp_param.output_frames = out_size;
    src_simple(&dsp_param,SRC_SINC_BEST_QUALITY,1);
    int audio_size = sampleBytes * out_size;
    QByteArray audio_out;
    audio_out.resize(audio_size);
    unsigned char *out_ptr = reinterpret_cast<unsigned char *>(audio_out.data());
    for (size_t i=0; i<out_size; i++)
    {
        for (int j=0; j<channelCount; j++)
        {
            if (sampleSize == 8)
            {
                if (m_format_out.sampleType() == QAudioFormat::UnSignedInt)
                {
                    const quint8 value = static_cast<quint8>((1.0 + outsample[i]) / 2 * 0xFF);
                    *reinterpret_cast<quint8 *>(out_ptr) = value;
                }
                else if (m_format_out.sampleType() == QAudioFormat::SignedInt)
                {
                    const qint8 value = static_cast<qint8>(outsample[i] * 0x7F);
                    *reinterpret_cast<qint8 *>(out_ptr) = value;
                }
            }
            else if (sampleSize == 16)
            {
                if (m_format_out.sampleType() == QAudioFormat::UnSignedInt)
                {
                    quint16 value = static_cast<quint16>((1.0 + outsample[i]) / 2 * 0xFFFF);
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint16>(value, out_ptr);
                    else
                        qToBigEndian<quint16>(value, out_ptr);
                }
                else if (m_format_out.sampleType() == QAudioFormat::SignedInt)
                {
                    qint16 value = static_cast<qint16>(outsample[i] * 0x7FFF);
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint16>(value, out_ptr);
                    else
                        qToBigEndian<qint16>(value, out_ptr);
                }
            }
            else if (sampleSize == 24)
            {
                if (m_format_out.sampleType() == QAudioFormat::UnSignedInt)
                {
                    quint32 value = static_cast<quint32>((1.0 + outsample[i]) / 2 * 0xFFFFFF);
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint32>(value, out_ptr);
                    else
                        qToBigEndian<quint32>(value, out_ptr);
                }
                else if (m_format_out.sampleType() == QAudioFormat::SignedInt)
                {
                    qint32 value = static_cast<qint32>(outsample[i] * 0x7FFFFF);
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint32>(value, out_ptr);
                    else
                        qToBigEndian<qint32>(value, out_ptr);
                }
            }
            else if (sampleSize == 32)
            {
                if (m_format_out.sampleType() == QAudioFormat::UnSignedInt)
                {
                    quint32 value = static_cast<quint32>((1.0 + outsample[i]) / 2 * static_cast<quint32>(0xFFFFFFFF));
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<quint32>(value, out_ptr);
                    else
                        qToBigEndian<quint32>(value, out_ptr);
                }
                else if (m_format_out.sampleType() == QAudioFormat::SignedInt)
                {
                    qint32 value = static_cast<qint32>(outsample[i] * static_cast<qint32>(0x7FFFFFFF));//static_cast<qint32>(0x8FFFFFFF)
                    if (m_format_out.byteOrder() == QAudioFormat::LittleEndian)
                        qToLittleEndian<qint32>(value, out_ptr);
                    else
                        qToBigEndian<qint32>(value, out_ptr);
                }
            }
            out_ptr += channelBytes;
        }
    }
    //send Audio to output
    m_outdev->write((const char *) audio_out, audio_size);

    //Get max audio peak
    for(uint32_t i = 0; i < s; ++i){
        if(pcm[i] > m_maxlevel){
            m_maxlevel = pcm[i];
        }
    }
    delete[] outsample;
    delete[] insample;
}

uint16_t AudioEngine::read(int16_t *pcm, int s)
{
    if(m_audioinq.size() >= s){
        for(int i = 0; i < s; ++i){
            pcm[i] = m_audioinq.dequeue();
        }
        return 1;
    }
    else{
        //fprintf(stderr, "audio frame not avail size == %d\n", m_audioinq.size());
        return 0;
    }
}

uint16_t AudioEngine::read(int16_t *pcm)
{
    int s;
    if(m_audioinq.size() >= 160){
        s = 160;
    }
    else{
        s = m_audioinq.size();
    }

    for(int i = 0; i < s; ++i){
        pcm[i] = m_audioinq.dequeue();
    }
    return s;
}
