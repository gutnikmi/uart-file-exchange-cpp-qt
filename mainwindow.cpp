
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "bytework.h"

#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include <chrono>

static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_settings(new SettingsDialog(this)),
    m_timer(new QTimer(this)),
    m_serial(new QSerialPort(this))
{
    m_ui->setupUi(this);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->statusBar->addWidget(m_status);
    initActionsConnections();
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
    m_timer->setSingleShot(true);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(m_serial, &QSerialPort::bytesWritten, this, &MainWindow::handleBytesWritten);
}
MainWindow::~MainWindow()
{
    delete m_settings;
    delete m_ui;
}
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
                          .arg(p.name, p.stringBaudRate, p.stringDataBits,
                               p.stringParity, p.stringStopBits, p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());

        showStatusMessage(tr("Open error"));
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
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About UartFiles"),
                       tr("The <b>UartFiles</b> program is used to send and recieve files "
                          "from in.txt to out.txt, both files have to be in the same "
                          "folder as  UartFiles.exe."));
}

void MainWindow::writeData(const QByteArray &data)
{
    const qint64 written = m_serial->write(data);
    if (written == data.size()) {
        m_bytesToWrite += written;
        m_timer->start(kWriteTimeout);
    } else {
        const QString error = tr("Failed to write all data to port %1.\n"
                                 "Error: %2").arg(m_serial->portName(),
                                                  m_serial->errorString());
        showWriteError(error);
    }
}

void MainWindow::readData()
{
    byteArray.append(m_serial->readAll());
    int length = byteArray.length();
    if (length >= 6)
    {
        if (byteArray[length-1] == '3' &&  byteArray[length-2] == '0' &&
            byteArray[length-3] == '7' && byteArray[length-4] == '0')
        {
            if (!(byteArray[length-5] == '7' && byteArray[length-6] == '0'))
            {
                receivedString = QString::fromUtf8(byteArray);
                m_ui->Received->append("Received string:");
                m_ui->Received->append(receivedString);
                std::string res = receivedString.toStdString();
                res = unpack_bytes(res);
                receivedString = QString::fromStdString(res);
                m_ui->Received->append("Unpacked string:");
                m_ui->Received->append(receivedString);
                m_ui->Received->append(QString::fromStdString(crc8_res));
                m_ui->Received->append("Writing to out.txt");
                writeToFile(receivedString);


                receivedString.clear();
                byteArray.clear();
            }
        }
    }
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::handleBytesWritten(qint64 bytes)
{
    m_bytesToWrite -= bytes;
    if (m_bytesToWrite == 0)
        m_timer->stop();
}

void MainWindow::handleWriteTimeout()
{
    const QString error = tr("Write operation timed out for port %1.\n"
                             "Error: %2").arg(m_serial->portName(),
                                              m_serial->errorString());
    showWriteError(error);
}

void MainWindow::clear()
{
    m_ui->Received->setText("");
}

void MainWindow::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(m_ui->actionClear, &QAction::triggered, this, &MainWindow::clear);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}


void MainWindow::showWriteError(const QString &message)
{
    QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::on_pushButton_clicked()
{
    m_ui->Received->append("Reading from in.txt");
    QString filestr = readFromFile();
    m_ui->Received->append("Read string:");
    m_ui->Received->append(filestr);
    std::string temp_str = filestr.toStdString();
    unsigned char str_char [temp_str.length()/2];
    convert_hex(str_char, temp_str.length(), temp_str.c_str());
    int size_b = (sizeof(str_char)/sizeof(*str_char));
    int size_p = size_b + count_DLE(str_char, size_b)+5;
    unsigned char out[size_p];
    pack_bytes(str_char, out, size_b);
    std::string str_data = convert_to_string(out, size_p);
    m_ui->Received->append("Packed bytes:");
    QString qstr = QString::fromStdString(str_data);
    m_ui->Received->append(qstr);
    QByteArray data = qstr.toUtf8();
    m_ui->Received->append("Sending data");
    writeData(data);
}


