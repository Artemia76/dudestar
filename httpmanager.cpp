#include "httpmanager.h"

HttpManager::HttpManager(QString f) : QObject(nullptr)
{
    m_qnam = new QNetworkAccessManager(this);
    QObject::connect(m_qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(http_finished(QNetworkReply*)));
    m_config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    m_filename = f;
#ifndef Q_OS_WIN
    m_config_path += "/dudestar";
#endif
}

void HttpManager::process()
{
    QMetaObject::invokeMethod(this,"doRequest");
#ifdef QT_DEBUG
    qDebug() << "process() called";
#endif
    //send to the event loop
}

void HttpManager::doRequest()
{
    m_qnam->get(QNetworkRequest(QUrl("http://www.dudetronics.com/ar-dns" + m_filename)));
#ifdef QT_DEBUG
    qDebug() << "doRequest() called m_filename == " << m_filename;
#endif
}

void HttpManager::http_finished(QNetworkReply *reply)
{
#ifdef QT_DEBUG
    qDebug() << "http_finished() called";
#endif
    if (reply->error()) {
        reply->deleteLater();
        reply = nullptr;
#ifdef QT_DEBUG
        qDebug() << "http_finished() error()";
#endif
        return;
    }
    else{
        QFile *hosts_file = new QFile(m_config_path + m_filename);
        hosts_file->open(QIODevice::WriteOnly);
        QFileInfo fileInfo(hosts_file->fileName());
        QString filename(fileInfo.fileName());
        hosts_file->write(reply->readAll());
        hosts_file->flush();
        hosts_file->close();
        delete hosts_file;
        emit file_downloaded(filename);
        fprintf(stderr, "Downloaded %s\n", filename.toStdString().c_str());fflush(stderr);
    }
    QThread::currentThread()->quit();
}
