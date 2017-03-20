#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QDebug>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mProcess{new Gdb("debug/gdb/gdb.exe")}
{
    ui->setupUi(this);

    connect(mProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(slotReadOutput()), Qt::UniqueConnection);
    connect(mProcess, SIGNAL(readyReadStandardError()), this, SLOT(slotReadOutput()), Qt::UniqueConnection);
    connect(mProcess, SIGNAL(signalLocalVarRecieved(QString)), this, SLOT(slotReadLocalVar(QString)), Qt::UniqueConnection);
    connect(ui->command, SIGNAL(returnPressed()), this, SLOT(slotWriteToProcess()), Qt::UniqueConnection);
    connect(ui->butLocalVar, SIGNAL(clicked(bool)), this, SLOT(slotGetLocalVar()), Qt::UniqueConnection);
    connect(ui->butRun, SIGNAL(clicked(bool)), this, SLOT(slotRun()), Qt::UniqueConnection);
    connect(ui->butStepOver, SIGNAL(clicked(bool)), this, SLOT(slotStepOver()), Qt::UniqueConnection);
    connect(ui->butBreakPoint, SIGNAL(clicked(bool)), this, SLOT(slotSetBreakPoint()), Qt::UniqueConnection);
    connect(ui->butClearBreakpoint, SIGNAL(clicked(bool)), this, SLOT(slotClearBreakPoint()), Qt::UniqueConnection);
    connect(ui->butStepIn, SIGNAL(clicked(bool)), this, SLOT(slotStepIn()), Qt::UniqueConnection);
    connect(ui->butStepOut, SIGNAL(clicked(bool)), this, SLOT(slotStepOut()), Qt::UniqueConnection);
    connect(ui->butCurrLine, SIGNAL(clicked(bool)), this, SLOT(slotCurrentLine()), Qt::UniqueConnection);


    QFile file(qApp->applicationDirPath().append("/gdb/gdb.exe"));
    qDebug() << "File exist: " << (file.exists());
    mProcess->start(QStringList() << "--interpreter=mi");

//    ui->command->setText("target exec debug/gdb/compl.exe");
    mProcess->openProject("debug/gdb/compl.exe");
    ui->command->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// target exec D:\Studying\Programming\Qt\My Project\build-UiDebuggerGdb-Custom_Kit-Debug\debug\gdb\gdb.exe

void MainWindow::slotReadOutput()
{
    QString processDataString(mProcess->getOutput());
    ui->echo->appendPlainText(processDataString);
}

void MainWindow::slotWriteToProcess()
{
    QByteArray data;
//    data.append(ui->command->text());
//    data.append('\n');
    QByteArray enter("\n");
    data.append(ui->command->text());
    //data.append(enter);
    mProcess->write(data);

    ui->command->clear();
}

void MainWindow::slotGetLocalVar()
{
    mProcess->getLocalVar();

    //ui->echo->appendPlainText(mProcess->peekLocalVar());
}

void MainWindow::slotReadLocalVar(const QString &str)
{
    ui->echo->appendPlainText("\n*********************\t\t\t{\n");
    auto lst = str.split("\\n");
    for(QString i : lst)
    {
        i.replace("\"", "");
        i.replace("~", "");
        ui->echo->appendPlainText(i);
        ui->echo->appendPlainText("\n");
    }
    ui->echo->appendPlainText("\n*********************\t\t\t}\n");
}

void MainWindow::slotRun()
{
    mProcess->run();
}

void MainWindow::slotStepOver()
{
    mProcess->stepOver();
}

void MainWindow::slotSetBreakPoint()
{
    mProcess->setBreakPoint(40);
}

void MainWindow::slotClearBreakPoint()
{
    mProcess->clearBreakPoint(40);
}

void MainWindow::slotStepIn()
{
    mProcess->stepIn();
}

void MainWindow::slotStepOut()
{
    mProcess->stepOut();
}

void MainWindow::slotCurrentLine()
{
    qDebug() << "Current line is:" << mProcess->getCurrentLine();
}
