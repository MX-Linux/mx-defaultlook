/**********************************************************************
 *  defaultlook.h
 **********************************************************************
 * Copyright (C) 2015 MX Authors
 *
 * Authors: Dolphin Oracle
 *          MX Linux <http://mxlinux.org>
 *
 * This file is part of mx-defaultlook.
 *
 * mx-welcome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mx-welcome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mx-defaultlook.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef DEFAULTLOOK_H
#define DEFAULTLOOK_H

#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QFile>

namespace Ui {
class defaultlook;
}

struct Result {
    int exitCode;
    QString output;
};

class defaultlook : public QMainWindow
{
    Q_OBJECT

protected:
    QProcess *proc;
    QTimer *timer;

public:
    explicit defaultlook(QWidget *parent = 0);
    ~defaultlook();
    Result runCmd(QString cmd);
    QString getVersion(QString name);

    QString version;
    QString output;
    QString panel;

    void setup();
    void setupuiselections();
    void fliptohorizontal();
    void fliptovertical();
    void whichpanel();
    void message();

public slots:




private slots:
    void on_buttonApply_clicked();

    void on_checkLightTheme_clicked();

    void on_checkDarkTheme_clicked();

    void on_buttonCancel_clicked();

    void on_buttonAbout_clicked();

    void on_buttonHelp_clicked();

private:
    Ui::defaultlook *ui;
};

#endif // DEFAULTLOOK_H
