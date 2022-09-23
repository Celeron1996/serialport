/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"

#include <QLabel>
#include <QMessageBox>

#include <QDateTime>
#include <QFileDialog>

#include <qvalidator.h>
#include <QTimer>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_settings(new SettingsDialog),
    m_serial(new QSerialPort(this)),
    Timer(new QTimer)
{
    m_ui->setupUi(this);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->statusBar->addWidget(m_status);
    initActionsConnections();

    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(Timer, &QTimer::timeout, this, &MainWindow::on_timeout);

    showStatusMessage(tr("Disconnected"));
    m_ui->pushButton_Send->setDisabled(true);

    QLabel *permanent=new QLabel(this);
    permanent->setText(tr("<a href=\"https://www.baidu.com\">百度一下</a>"));
    permanent->setOpenExternalLinks(true);//设置可以打开网站链接
    m_ui->statusBar->addPermanentWidget(permanent);

    QLabel *permanent2=new QLabel(this);
    permanent2->setText(tr("<a href=\"https://www.taobao.com\">打开淘宝</a>"));
    permanent2->setOpenExternalLinks(true);//设置可以打开网站链接
    m_ui->statusBar->addPermanentWidget(permanent2);

//    m_ui->lineEdit_Timing->setValidator(new QIntValidator(0,1000,this));
    QRegExp regx("[0-9]+$");
    QValidator *validator = new QRegExpValidator(regx, m_ui->lineEdit_Timing);
    m_ui->lineEdit_Timing->setValidator(validator);
    m_ui->lineEdit_Timing->setText("1000");
    m_ui->checkBox_LoopSend->setCheckState(Qt::Unchecked);
    m_ui->checkBox_LoopSend->setEnabled(false);

}


MainWindow::~MainWindow()
{
    delete m_settings;
    delete m_ui;
}

//! [4]
void MainWindow::openSerialPort()
{
    const SettingsDialog::Settings p = m_settings->settings();
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        m_ui->pushButton_Send->setDisabled(false);
        m_ui->checkBox_LoopSend->setEnabled(true);
    } else {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());

        showStatusMessage(tr("Open error"));

        m_ui->pushButton_Send->setDisabled(true);
    }
}


void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
    m_ui->pushButton_Send->setDisabled(true);
    m_ui->checkBox_LoopSend->setCheckState(Qt::Unchecked);
    m_ui->checkBox_LoopSend->setEnabled(false);
}


void MainWindow::about()
{
    QMessageBox::about(this, tr("关于此软件"),
                       tr("<b>超级串口调试大师</b> 是一个免费的串口调试软件"));
}


void MainWindow::writeData(const QByteArray &data)
{
    m_serial->write(data);

    DisPlay_textBrowser_Receive(data, DisplayModel_Send);
}


void MainWindow::readData()
{
    const QByteArray data = m_serial->readAll();
    DisPlay_textBrowser_Receive(data, DisplayModel_Receive);
}


void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}


void MainWindow::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionClear, &QAction::triggered, this, &MainWindow::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    //connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}

void MainWindow::clear()
{
    m_ui->textBrowser_Receive->clear();
}

void MainWindow::on_pushButton_Send_clicked()
{
    QString str = m_ui->textEdit_Send->toPlainText();
    QByteArray array;

    if(str == "")
    {
        QMessageBox::critical(this, tr("Error"), "数据发送区为空！");
        return ;
    }

    if(m_ui->checkBox_HexSend->isChecked())
    {
        array = QByteArray::fromHex(str.toLatin1().data());
    }
    else
    {
        array = str.toUtf8();
    }

    if(m_ui->checkBox_SendNewLine->isChecked())
    {
        array.append(0x0D);
        array.append(0x0A);
    }
    writeData(array);
}

void MainWindow::on_pushButton_Save_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存串口数据"), "mydata",
        tr("txt file (*.txt);;All Files (*)"));

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                file.errorString());
            return;
        }

        QString text = m_ui->textBrowser_Receive->toPlainText();
        char *save_text;
        QByteArray temp = text.toLatin1();
        save_text = temp.data();
        file.write(save_text, temp.count());
        file.close();
    }
}

void MainWindow::on_checkBox_LoopSend_stateChanged(int arg1)
{
    QString str = m_ui->textEdit_Send->toPlainText();

    if(arg1 == 0)
    {
        Timer->stop();
    }
    else
    {
        if(m_ui->lineEdit_Timing->text().toInt() == 0)
        {
            QMessageBox::critical(this, tr("Error"), "定时时间不能为0！");
            m_ui->checkBox_LoopSend->setCheckState(Qt::Unchecked);
        }
        else if(m_ui->pushButton_Send->isEnabled() == false)
        {
            QMessageBox::critical(this, tr("Error"), "请先连接串口！");
            m_ui->checkBox_LoopSend->setCheckState(Qt::Unchecked);
        }
        else if(str == "")
        {
            QMessageBox::critical(this, tr("Error"), "数据发送区为空！");
            m_ui->checkBox_LoopSend->setCheckState(Qt::Unchecked);
        }
        else
        {
            Timer->start(m_ui->lineEdit_Timing->text().toInt());
        }
    }
}


void MainWindow::on_timeout()
{
    QString str = m_ui->textEdit_Send->toPlainText();
    QByteArray array;

    if(m_ui->checkBox_HexSend->isChecked())
    {
        array = QByteArray::fromHex(str.toLatin1().data());
    }
    else
    {
        array = str.toUtf8();
    }

    if(m_ui->checkBox_SendNewLine->isChecked())
    {
        array.append(0x0D);
        array.append(0x0A);
    }
    writeData(array);
}

void MainWindow::DisPlay_textBrowser_Receive(const QByteArray &data, DisplayModel Model)
{
    m_ui->textBrowser_Receive->moveCursor(QTextCursor::End);
    QString str;
    QTime time;
    time = time.currentTime();
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();
    int msec = time.msec();
    QString timestr;
    if(Model == DisplayModel_Send)
    {
        timestr = "[" + QString("%1").arg(hour, 2, 10, QChar('0')) + ":" + QString("%1").arg(minute, 2, 10, QChar('0')) + ":" + QString("%1").arg(second, 2, 10, QChar('0')) + ":" + QString("%1").arg(msec, 3, 10, QChar('0')) + "]O->:";
    }
    else
    {
        timestr = "[" + QString("%1").arg(hour, 2, 10, QChar('0')) + ":" + QString("%1").arg(minute, 2, 10, QChar('0')) + ":" + QString("%1").arg(second, 2, 10, QChar('0')) + ":" + QString("%1").arg(msec, 3, 10, QChar('0')) + "]O<-:";
    }


    if(m_ui->checkBox_HexDisplay->isChecked() == true)
    {
        QString strtemp = data.toHex().data();
        strtemp = strtemp.toUpper();
        for(int i = 0; i < strtemp.length(); i+=2)
        {
            QString strnull = strtemp.mid(i, 2);
            str += strnull;
            str += " ";
        }
    }
    else
    {
        str = data;
    }
    if(m_ui->checkBox_Time->isChecked() == true)
    {
        str = timestr + str;
    }
    if(m_ui->checkBox_AutoEnter->isChecked() == true)
    {
        m_ui->textBrowser_Receive->append(str);
    }
    else
    {
        m_ui->textBrowser_Receive->insertPlainText(str);
    }

    //this->m_ui->textBrowser_Receive->append(str);
    //this->m_ui->textBrowser_Receive->insertPlainText(str);
    m_ui->textBrowser_Receive->moveCursor(QTextCursor::End);
}
