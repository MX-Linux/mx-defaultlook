/**********************************************************************
 *  defaultlook.cpp
 **********************************************************************
 * Copyright (C) 2015 MX Authors
 *
 * Authors: Dolphin Oracle
 *         MX Linux <http://mxlinux.org>
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

#include "defaultlook.h"
#include "ui_defaultlook.h"
#include "QDebug"
#include "QDir"

defaultlook::defaultlook(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::defaultlook)
{
    ui->setupUi(this);
    setup();
}

defaultlook::~defaultlook()
{
    delete ui;
}

// setup versious items first time program runs
void defaultlook::setup()
{
    this->setWindowTitle(tr("MX Default Look"));
    this->adjustSize();
    ui->buttonApply->setEnabled(false);
    checkXFCE();
    whichpanel();
    message_flag = false;
    QString cmd = QString("test -f ~/.restore/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml");
    if (system(cmd.toUtf8()) != 0) {
        backupPanel();
        message2();
    }
    setupuiselections();
}

// Util function for getting bash command output and error code
Result defaultlook::runCmd(QString cmd)
{
    QEventLoop loop;
    proc = new QProcess(this);
    proc->setReadChannelMode(QProcess::MergedChannels);
    connect(proc, SIGNAL(finished(int)), &loop, SLOT(quit()));
    proc->start("/bin/bash", QStringList() << "-c" << cmd);
    loop.exec();
    disconnect(proc, 0, 0, 0);
    Result result = {proc->exitCode(), proc->readAll().trimmed()};
    delete proc;
    return result;
}

void defaultlook::setupuiselections()
{
    //reset all checkboxes to unchecked

    ui->checkVert->setChecked(false);
    ui->checkHorz->setChecked(false);
    ui->radioBackupPanel->setChecked(false);
    ui->radioDefaultPanel->setChecked(false);
    ui->radioRestoreBackup->setChecked(false);
    ui->checkLightTheme->setChecked(false);
    ui->checkDarkTheme->setChecked(false);
    ui->checkFirefox->setChecked(false);
    ui->checkHexchat->setChecked(false);

    //only enable options that make sense

    //if panel is already horizontal, set vertical option available, and vice versa

    QString test = runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/mode").output;

    if (test == "") {
        ui->checkVert->setEnabled(true);
        ui->checkHorz->setChecked(true);
        ui->comboboxHorzPostition->setEnabled(true);
    }
    if (test == "0") {
        ui->checkVert->setEnabled(true);
        ui->checkHorz->setChecked(true);
        ui->comboboxHorzPostition->setEnabled(true);
    }
    if (test == "1") {
        ui->checkVert->setChecked(true);
        ui->checkHorz->setEnabled(true);
        ui->comboboxHorzPostition->setEnabled(true);
    }

    if (test == "2") {
        ui->checkVert->setChecked(true);
        ui->checkHorz->setEnabled(true);
        ui->comboboxHorzPostition->setEnabled(true);
    }

    // if backup available, make the restore backup option available

    QString cmd = QString("test -f ~/.restore/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml");
    if (system(cmd.toUtf8()) == 0) {
        ui->radioRestoreBackup->setEnabled(true);
    } else {
        ui->radioRestoreBackup->setEnabled(false);
    }
    // check theme overrides

    QString home_path = QDir::homePath();
    QFileInfo file(home_path + "/.config/FirefoxDarkThemeOverride.check");
    if (file.exists()) {
        ui->checkFirefox->setChecked(true);
    }
    //check status of hex chat dark tweak
    QFileInfo file_hexchat(home_path + "/.config/hexchat/hexchat.conf");
    if (file_hexchat.exists()) {
        //check for absolutePath()setting
       QString code = runCmd("grep 'gui_input_style = 0' " + file_hexchat.absoluteFilePath()).output;
        qDebug() << "hexchat command :" << code;
        if (code == "gui_input_style = 0") {
            ui->checkHexchat->setChecked(true);
        }
    }
}

void defaultlook::whichpanel()
{
    // take the first panel we see as default
    QString panel_content;
    QStringList panelIDs;
    panel_content = runCmd("LC_ALL=en_US.UTF-8 xfconf-query -c xfce4-panel -p /panels | grep -v Value | grep -v ^$").output;
    panelIDs = panel_content.split("\n");
    panel = panelIDs.value(0);
    qDebug() << "panel to use: " << panel;
}

void defaultlook::fliptohorizontal()
{
    QString file_content;
    QStringList pluginIDs;
    file_content = runCmd("LC_ALL=en_US.UTF-8 xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/plugin-ids | grep -v Value | grep -v ^$").output;
    pluginIDs = file_content.split("\n");
    qDebug() << pluginIDs;

    // figure out moving the systray, if it exists

    // figure out systrayID, and tasklistID

    QString systrayID = runCmd("grep systray ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
    systrayID=systrayID.remove("\"").section("-",1,1).section(" ",0,0);
    qDebug() << "systray: " << systrayID;

    QString tasklistID = runCmd("grep tasklist ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
    tasklistID=tasklistID.remove("\"").section("-",1,1).section(" ",0,0);
    qDebug() << "tasklist: " << tasklistID;

    // if systray exists, do a bunch of stuff to relocate it a list of plugins.  If not present, do nothing to list

    if (systrayID !=""){

        //get tasklist index in list
        int tasklistindex = pluginIDs.indexOf(tasklistID);
        qDebug() << "tasklistIDindex 1" << tasklistindex;

        //check next plugin in list to see if its an expanding separator
        int expsepindex = tasklistindex + 1;
        qDebug() << "expsepindex" << expsepindex;
        QString expsepID = pluginIDs.value(expsepindex);
        qDebug() << "expsepID to test" << expsepID;
        QString test = runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + expsepID + "/expand").output;
        qDebug() << "test parm" << test;


        //move the notification area (systray) to above window buttons (tasklist) in the list if tasklist exists

        if (tasklistID !="") {
            pluginIDs.removeAll(systrayID);
            tasklistindex = pluginIDs.indexOf(tasklistID);
            qDebug() << "tasklistIDindex 2" << tasklistindex;
            pluginIDs.insert(tasklistindex, systrayID);
            qDebug() << "reordered list" << pluginIDs;

            //move the expanding separator

            if (test == "true") {
                pluginIDs.removeAll(expsepID);
                tasklistindex = pluginIDs.indexOf(tasklistID);
                qDebug() << "tasklistIDindex 2" << tasklistindex;
                pluginIDs.insert(tasklistindex, expsepID);
                qDebug() << "reordered list" << pluginIDs;
            }
        }

        //if the tasklist isn't present, try to make a decision about where to put the systray

        if (tasklistID == "") {


            //try to move to in front of clock if present

            QString clockID = runCmd("grep clock ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
            QString switchID;
            clockID=clockID.remove("\"").section("-",1,1).section(" ",0,0);
            qDebug() << "clockID: " << clockID;
            if (clockID != "") {
                switchID = clockID;

                //if clock found check if next plugin down is a separator and if so put it there

                int clocksepindex = pluginIDs.indexOf(clockID) + 1;
                QString clocksepcheck = runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + pluginIDs.value(clocksepindex)).output;
                qDebug() << "clocksepcheck: " << clocksepcheck;
                if (clocksepcheck == "separator") {
                    switchID = pluginIDs.value(clocksepindex);
                }

                // if there is no clock, put it near the end and hope for the best

            } else {
                switchID = pluginIDs.value(1);
            }

            // move the systray

            int switchIDindex;
            pluginIDs.removeAll(systrayID);
            switchIDindex = pluginIDs.indexOf(switchID) + 1;
            qDebug() << "switchIDindex 2" << switchIDindex;
            pluginIDs.insert(switchIDindex, systrayID);
            qDebug() << "reordered list" << pluginIDs;
        }
    }
    //now reverse the list

    std::reverse(pluginIDs.begin(), pluginIDs.end());
    qDebug() << "reversed list" << pluginIDs;

    //now build xfconf command

    QStringListIterator changeIterator(pluginIDs);
    QString cmdstring;
    while (changeIterator.hasNext()) {
        QString value = changeIterator.next();
        cmdstring = QString(cmdstring + "-s " + value + " ");
        qDebug() << cmdstring;
    }

    //flip the panel plugins and hold on, it could be a bumpy ride

    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/plugin-ids " + cmdstring);

    //change orientation to horizontal

    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/mode -s 0");

    //change mode of tasklist labels if it exists

    if (tasklistID != "") {
        runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + tasklistID + "/show-labels -s true");
    }
    //deteremine top or bottom horizontal placement
    top_or_bottom();

    runCmd("xfce4-panel --restart");
}

void defaultlook::fliptovertical()
{
    QString file_content;
    QStringList pluginIDs;
    file_content = runCmd("LC_ALL=en_US.UTF-8 xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/plugin-ids | grep -v Value | grep -v ^$").output;
    pluginIDs = file_content.split("\n");
    qDebug() << pluginIDs;


    // figure out moving the systray, if it exists

    QString systrayID = runCmd("grep systray ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
    systrayID=systrayID.remove("\"").section("-",1,1).section(" ",0,0);
    qDebug() << "systray: " << systrayID;

    QString tasklistID = runCmd("grep tasklist ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
    tasklistID=tasklistID.remove("\"").section("-",1,1).section(" ",0,0);
    qDebug() << "tasklist: " << tasklistID;

    //if systray exists, do a bunch of stuff to try to move it in a logical way

    if (systrayID !=""){

        // figure out whiskerID, appmenuID, systrayID, tasklistID, and pagerID

        QString whiskerID = runCmd("grep whisker ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
        whiskerID=whiskerID.remove("\"").section("-",1,1).section(" ",0,0);
        qDebug() << "whisker: " << whiskerID;

        QString pagerID = runCmd("grep pager ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
        pagerID=pagerID.remove("\"").section("-",1,1).section(" ",0,0);
        qDebug() << "pager: " << pagerID;

        QString appmenuID = runCmd("grep applicationsmenu ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml").output;
        appmenuID=appmenuID.remove("\"").section("-",1,1).section(" ",0,0);
        qDebug() << "appmenuID: " << appmenuID;

        //get tasklist index in list
        int tasklistindex = pluginIDs.indexOf(tasklistID);
        qDebug() << "tasklistIDindex 1" << tasklistindex;

        //check next plugin in list to see if its an expanding separator
        int expsepindex = tasklistindex + 1;
        qDebug() << "expsepindex" << expsepindex;
        QString expsepID = pluginIDs.value(expsepindex);
        qDebug() << "expsepID to test" << expsepID;
        QString testexpandsep = runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + expsepID + "/expand").output;
        qDebug() << "test parm" << testexpandsep;


        //move the notification area (systray) to an appropriate area.

        //1.  determine if menu is present, place in front of menu

        QString switchID;
        if (whiskerID != "") {
            switchID = whiskerID;
            qDebug() << "switchID whisker: " << switchID;
        } else {
            if (appmenuID != "") {
                switchID = appmenuID;
                qDebug() << "switchID appmenu: " << switchID;
            }
        }

        //2.  if so, check second plugin is separator, if so place in front of separator

        if (switchID != "") {
            QString test = runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + pluginIDs.value(1)).output;
            if (test == "separator") {
                qDebug() << "test parm" << test;
                switchID = pluginIDs.value(1);
                qDebug() << "switchID sep: " << switchID;
            }
        }

        //3.  if so, check third plugin is pager.  if so, place tasklist in front of pager

        if (switchID != ""){
            QString test = runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + pluginIDs.value(2)).output;
            if (test == "pager") {
                qDebug() << "test parm" << test;
                switchID = pluginIDs.value(2);
                qDebug() << "switchID pager: " << switchID;
            }
        }

        // if the menu doesn't exist, give a default value that is sane but might not be correct

        if (switchID == "") {
            switchID = pluginIDs.value(1);
            qDebug() << "switchID default: " << switchID;
        }


        //4.  move the systray

        pluginIDs.removeAll(systrayID);
        int switchindex = pluginIDs.indexOf(switchID) + 1;
        qDebug() << "switchindex" << switchindex;
        pluginIDs.insert(switchindex, systrayID);
        qDebug() << "reordered list" << pluginIDs;

        //move the expanding separator

        if (testexpandsep == "true") {
            pluginIDs.removeAll(expsepID);
            tasklistindex = pluginIDs.indexOf(tasklistID);
            qDebug() << "tasklistIDindex 2" << tasklistindex;
            pluginIDs.insert(tasklistindex, expsepID);
            qDebug() << "reordered list" << pluginIDs;
        }
    }

    //now reverse the list

    std::reverse(pluginIDs.begin(), pluginIDs.end());
    qDebug() << "reversed list" << pluginIDs;

    //now build xfconf command

    QStringListIterator changeIterator(pluginIDs);
    QString cmdstring;
    while (changeIterator.hasNext()) {
        QString value = changeIterator.next();
        cmdstring = QString(cmdstring + "-s " + value + " ");
        qDebug() << cmdstring;
    }

    //flip the panel plugins and pray for a miracle


    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/plugin-ids " + cmdstring);

    //change orientation to vertical

    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel + "/mode -n -t int -s 2");
    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel + "/position -s 'p=5;x=0;y=0'");

    //change mode of tasklist labels if they exist

    if (tasklistID != "") {
        runCmd("xfconf-query -c xfce4-panel -p /plugins/plugin-" + tasklistID + "/show-labels -s false");
    }

    //restart xfce4-panel

    system("xfce4-panel --restart");
}

//// slots ////

//Apply Button
void defaultlook::on_buttonApply_clicked()
{
    ui->buttonApply->setEnabled(false);

    //backups and default panel
    if (ui->radioDefaultPanel->isChecked()) {
        restoreDefaultPanel();
        runCmd("sleep .5");
        whichpanel();
    }

    if (ui->radioRestoreBackup->isChecked()) {
        restoreBackup();
        runCmd("sleep .5");
        whichpanel();
    }

    if (ui->radioBackupPanel->isChecked()) {
        backupPanel();

    }
    //read in plugin ID's
    if (ui->checkHorz->isChecked()) {
        QString test = runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/mode").output;

        if (test == "1" || test =="2") {
            fliptohorizontal();
        }


        runCmd("sleep .5");
    }

    if (ui->checkVert->isChecked()) {
        QString test = runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/mode").output;

        if (test == "" || test =="0") {
            fliptovertical();
        }
        runCmd("sleep .5");
    }

    if (ui->checkDarkTheme->isChecked()) {
        message_flag = true;
        QFileInfo theme("/usr/share/themes/Adwaita-Xfce-Dark-Thick");
        if (theme.exists()) {
            runCmd("xfconf-query -c xsettings -p /Net/ThemeName -s 'Adwaita-Xfce-Dark-Thick'");
            runCmd("sleep .5");
            runCmd("xfconf-query -c xfwm4 -p /general/theme -s 'Adwaita-Xfce-Dark-Thick'");
            runCmd("sleep .5");
            QFileInfo icon("/usr/share/icons/Papirus-Dark-GTK");
            if (icon.exists()) {
                runCmd("xfconf-query -c xsettings -p /Net/IconThemeName -s 'Papirus-Dark-GTK'");
                runCmd("sleep .5");
            }
            //restart xfce4-panel

            system("xfce4-panel --restart");
        }

    }

    if (ui->checkLightTheme->isChecked()) {
        message_flag = true;
        QFileInfo theme("/usr/share/themes/Greybird-thick-grip");
        if (theme.exists()) {
            runCmd("xfconf-query -c xsettings -p /Net/ThemeName -s Greybird-mx16-thick-grip");
            runCmd("sleep .5");
            runCmd("xfconf-query -c xfwm4 -p /general/theme -s Greybird-mx16-thick-grip");
            runCmd("sleep .5");
            QFileInfo icon("/usr/share/icons/Papirus-GTK");
            if (icon.exists()) {
                runCmd("xfconf-query -c xsettings -p /Net/IconThemeName -s 'Papirus-GTK'");
                runCmd("sleep .5");
            }
            //restart xfce4-panel

            system("xfce4-panel --restart");
        }
    }

    if (ui->checkFirefox->isChecked()) {
        runCmd("touch /home/$USER/.config/FirefoxDarkThemeOverride.check");
    } else {
        runCmd("rm /home/$USER/.config/FirefoxDarkThemeOverride.check");
    }

    //deal with hexchat
    QString home_path = QDir::homePath();
    QFileInfo file_hexchat(home_path + "/.config/hexchat/hexchat.conf");
    if (ui->checkHexchat->isChecked()) {
        if (file_hexchat.exists()) {
            //replace setting
            runCmd("sed -i -r 's/gui_input_style = 1/gui_input_style = 0/' " + file_hexchat.absoluteFilePath());
        } else {
            //copy a config file into user directory
            runCmd("mkdir -p " + home_path + "/.config/hexchat");
            runCmd("cp /usr/share/mx-defaultlook/hexchat.conf " + file_hexchat.absoluteFilePath());
        }
    } else {
        if (file_hexchat.exists()) {
            //replace setting
            runCmd("sed -i -r 's/gui_input_style = 0/gui_input_style = 1/' " + file_hexchat.absoluteFilePath());
        }
    }


    // message that we are done if a theme change was made

    if (message_flag == true) {
        message();
        message_flag = false;
    }

    // reset gui
    setupuiselections();

}

void defaultlook::on_checkLightTheme_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->checkLightTheme->isChecked()) {
        ui->checkDarkTheme->setChecked(false);
        ui->checkFirefox->setChecked(false);
        ui->checkHexchat->setChecked(false);
    }
}

void defaultlook::on_checkDarkTheme_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->checkDarkTheme->isChecked()) {
        ui->checkLightTheme->setChecked(false);
        ui->checkHexchat->setChecked(true);
        ui->checkFirefox->setChecked(true);
    }
}

void defaultlook::on_buttonCancel_clicked()
{
    qApp->quit();
}

void defaultlook::on_buttonAbout_clicked()
{
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Default Look"), "<p align=\"center\"><b><h2>" +
                       tr("MX Default Look") + "</h2></b></p><p align=\"center\">" + tr("Version: ") + version + "</p><p align=\"center\"><h3>" +
                       tr("App for quick default ui theme changes") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>", 0, this);
    msgBox.addButton(tr("Cancel"), QMessageBox::AcceptRole); // because we want to display the buttons in reverse order we use counter-intuitive roles.
    msgBox.addButton(tr("License"), QMessageBox::RejectRole);
    if (msgBox.exec() == QMessageBox::RejectRole) {
        system("mx-viewer file:///usr/share/doc/mx-defaultlook/license.html '" + tr("MX Default Look").toUtf8() + " " + tr("License").toUtf8() + "'");
    }
    this->show();
}

// Help button clicked
void defaultlook::on_buttonHelp_clicked()
{
    this->hide();
    QString cmd = QString("mx-viewer https://mxlinux.org/wiki/help-files/help-mx-default-look '%1'").arg(tr("MX Default Look"));
    system(cmd.toUtf8());
    this->show();
}

void defaultlook::message()
{
    QString cmd = "ps -aux |grep -v grep|grep firefox";
    if ( system(cmd.toUtf8()) != 0 ) {
        qDebug() << "Firefox not running" ;
    } else {
        QMessageBox::information(0, tr("MX Default Look"),
                             tr("Finished! Firefox may require a restart for changes to take effect"));
    }
}

void defaultlook::checkXFCE()
{
    QString test = runCmd("echo $XDG_CURRENT_DESKTOP").output;
    qDebug() << test;
    if ( test != "XFCE") {
        QMessageBox::information(0, tr("MX Default Look"),
                                 tr("This app is Xfce-only"));
        qApp->quit();
    }
}

// backs up the current panel configuration
void defaultlook::backupPanel()
{
    runCmd("rm -Rf ~/.restore; mkdir -p ~/.restore/.config/xfce4; \
           mkdir -p ~/.restore/.config/xfce4/xfconf/xfce-perchannel-xml; \
            cp -Rf ~/.config/xfce4/panel ~/.restore/.config/xfce4; \
    cp -f ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml ~/.restore/.config/xfce4/xfconf/xfce-perchannel-xml/");
}

void defaultlook::restoreDefaultPanel()
{
    // copy template files
    runCmd("pkill xfconfd; rm -Rf ~/.config/xfce4/panel; cp -Rf /usr/local/share/appdata/panels/vertical/panel ~/.config/xfce4; \
           cp -f /usr/local/share/appdata/panels/vertical/xfce4-panel.xml ~/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml; \
             xfconfd; sleep 2; xfce4-panel --restart");
}

void defaultlook::restoreBackup()
{
    runCmd("pkill xfconfd; rm -Rf ~/.config/xfce4/panel; cp -Rf ~/.restore/.config/xfce4/panel ~/.config/xfce4; \
           cp -f ~/.restore/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml ~/.config/xfce4/xfconf/xfce-perchannel-xml; \
            xfconfd; sleep 2; xfce4-panel --restart");
}

void defaultlook::on_checkHorz_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->checkHorz->isChecked()) {
        ui->checkVert->setChecked(false);
        ui->radioBackupPanel->setChecked(false);
        ui->radioDefaultPanel->setChecked(false);
        ui->radioRestoreBackup->setChecked(false);
    }


}

void defaultlook::on_checkVert_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->checkVert->isChecked()) {
        ui->checkHorz->setChecked(false);
        ui->radioBackupPanel->setChecked(false);
        ui->radioDefaultPanel->setChecked(false);
        ui->radioRestoreBackup->setChecked(false);
    }

}

void defaultlook::on_checkFirefox_clicked()
{
    ui->buttonApply->setEnabled(true);
    message_flag = true;
}

void defaultlook::on_checkHexchat_clicked()
{
    ui->buttonApply->setEnabled(true);
}


void defaultlook::on_radioDefaultPanel_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->radioDefaultPanel->isChecked()) {
        ui->checkHorz->setChecked(false);
        ui->radioBackupPanel->setChecked(false);
        ui->checkVert->setChecked(false);
        ui->radioRestoreBackup->setChecked(false);
    }
}

void defaultlook::on_radioBackupPanel_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->radioBackupPanel->isChecked()) {
        ui->checkHorz->setChecked(false);
        ui->checkVert->setChecked(false);
        ui->radioDefaultPanel->setChecked(false);
        ui->radioRestoreBackup->setChecked(false);
    }
}

void defaultlook::on_radioRestoreBackup_clicked()
{
    ui->buttonApply->setEnabled(true);
    if (ui->radioRestoreBackup->isChecked()) {
        ui->checkHorz->setChecked(false);
        ui->radioBackupPanel->setChecked(false);
        ui->radioDefaultPanel->setChecked(false);
        ui->checkVert->setChecked(false);
    }

}

void defaultlook::top_or_bottom()
{
    //move to user selected top or bottom border per mx-16 defaults  p=11 is top, p=12 is bottom

    QString top_bottom;
    if (ui->comboboxHorzPostition->currentIndex() == 0) {
        top_bottom = "12";
    }

    if (ui->comboboxHorzPostition->currentIndex() == 1) {
        top_bottom = "11";
    }

    qDebug() << "position index is : " << ui->comboboxHorzPostition->currentIndex();
    qDebug() << "position is :" << top_bottom;

    runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel + "/position -s 'p=" + top_bottom + ";x=0;y=0'");

}

void defaultlook::message2()
{
    QMessageBox::information(0, tr("Panel settings"),
                             tr(" Your current panel settings have been backed up in a hidden folder called .restore in your home folder (~/.restore/)"));
}

void defaultlook::on_toolButtonXFCEpanelSettings_clicked()
{
    this->hide();
    system("xfce4-panel --preferences");
    this->show();
}

void defaultlook::on_toolButtonXFCEAppearance_clicked()
{
    this->hide();
    system("xfce4-appearance-settings");
    this->show();
}

void defaultlook::on_toolButtonXFCEWMsettings_clicked()
{
    this->hide();
    system("xfwm4-settings");
    this->show();
}

void defaultlook::on_comboboxHorzPostition_currentIndexChanged(const QString &arg1)
{
    qDebug() << "top or bottom output " << ui->comboboxHorzPostition->currentText();
    QString test = runCmd("xfconf-query -c xfce4-panel -p /panels/panel-" + panel +"/mode").output;
    qDebug() << "test value, blank or 0 runs top_or_bottom" << test;
    if (test == "") {
        top_or_bottom();
    }
    if (test == "0") {
        top_or_bottom();
    }
}
