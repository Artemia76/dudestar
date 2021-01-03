/*
    Copyright (C) 2019 Doug McLain

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

#include "dudestar.h"
#include <QApplication>
#include <QStyleFactory>
#include <QTranslator>
#include <QSettings>

#ifdef Q_OS_MACOS
    #include "tools/keepalive.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_MACOS
    KeepAlive kalive;
#endif
    QApplication::setStyle("fusion");
    QCoreApplication::setOrganizationName("F4IKZ");
    QCoreApplication::setOrganizationDomain("F4IKZ");
    QCoreApplication::setApplicationName("Dudestar");
    QSettings settings;
    QString locale = QLocale::system().name();
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    QTranslator Trans;
#ifdef Q_OS_LINUX
    Trans.load("/usr/share/dudestar/translations/dudestar_" + locale +".qm");
#else
#ifdef Q_OS_MACOS
    Trans.load("../Resources/dudestar"+ locale +".qm");
#else
    Trans.load("translations/dudestar_" + locale +".qm");
#endif
#endif
    a.installTranslator(&Trans);
    DudeStar w;
    w.show();

    return a.exec();
}
