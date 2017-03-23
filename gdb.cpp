#include "gdb.h"

#include <QDebug>
#include <iostream>
#include <QRegExp>

Gdb::Gdb()
{

}

Gdb::Gdb(QString gdbPath):
    mCaptureLocalVar(false),
    mCaptureLocalVarSeveralTimes{0}
{
    mGdbFile.setFileName(gdbPath);
    connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(slotReadStdOutput()), Qt::UniqueConnection);
    connect(this, SIGNAL(readyReadStandardError()), this, SLOT(slotReadErrOutput()), Qt::UniqueConnection);
}

void Gdb::start(const QStringList &arguments, QIODevice::OpenMode mode)
{
    QProcess::start(mGdbFile.fileName(), arguments, mode);
}

void Gdb::write(QByteArray &command)
{
    QByteArray enter("\n");
    command.append(enter);
    QProcess::write(command);
}

void Gdb::readStdOutput()
{
    mBuffer = QProcess::readAll();
    //^error,msg="The program is not being run."
    QRegExp errorMatch("\\^error");
    if(errorMatch.indexIn(mBuffer) != -1)
    {
        QRegExp errorMsg("msg=[\\w\\s\"]+");
        errorMsg.indexIn(mBuffer);
        mErrorMessage = errorMsg.cap();
        emit signalErrorOccured(mErrorMessage);
    }
}

void Gdb::readErrOutput()
{
    //mBuffer = QProcess::readAllStandardError();

}

const QString &Gdb::getOutput() const
{
    return mBuffer;
}

QStringList Gdb::getLocalVar()
{   //finds and returns all local variable names
    write(QByteArray("info local"));
    QProcess::waitForReadyRead(1000);
    QRegExp varMatch("\"\\w+\\s=");
    qDebug() << ((varMatch.indexIn(mBuffer) == -1) ? "Nothing found":"Something found");
    int pos = 0;
    QStringList locals;
    while(pos != -1)
    {
        pos = varMatch.indexIn(mBuffer, pos+1);
        QRegExp clean("\"|\\s|=");
        QString varName = varMatch.cap().replace(clean, "").trimmed();
        if(!varName.isEmpty())
        {
            locals << varName;
        }
    }
    return locals;
}

const QString &Gdb::peekLocalVar() const
{
    return mLocalVar;
}

void Gdb::openProject(const QString &fileName)
{   //opens file $fileName$ in gdb to debug it via target exec and file
    write(QByteArray("target exec ").append(fileName));
    write(QByteArray("file ").append(fileName));
}

void Gdb::run()
{   //run debugging
    write(QByteArray("run"));
}

void Gdb::stepOver()
{   //goes to the next line of code
    write(QByteArray("next"));
}

void Gdb::setBreakPoint(unsigned int line)
{   //set simple breakpoint at line $line$
    write(QByteArray("b ").append(QString::number(line)));
}

void Gdb::clearBreakPoint(unsigned int line)
{
    write(QByteArray("clear ").append(QString::number(line)));
}

void Gdb::stepIn()
{   //step into function under cursor
    write(QByteArray("step"));
}

void Gdb::stepOut()
{   //step out of current function\method
    write(QByteArray("finish"));
}

int Gdb::getCurrentLine()
{   //returns current line of code or -1 if any aerror occured
    write(QByteArray("frame"));
    QProcess::waitForReadyRead();
    QRegExp rx(":\\d+");
    if(rx.indexIn(mBuffer) == -1)
    {
        return -1;
    }
    QStringList lst = rx.capturedTexts();
    QString line = lst[0];
    return line.split(':').last().toInt();
}

void Gdb::updateBreakpointsList()
{
    write(QByteArray("info b"));
    QProcess::waitForReadyRead(1000);
    QStringList lines = mBuffer.split('~');
    for(auto i : lines)
    {
        qDebug() << i << "\n";
    }
    mBreakpointsList.clear();

    QString topic = lines[0];
    for(int i=2;i<lines.size();++i)
    {
        Breakpoint addBrk;
        QString currentLine = lines[i];
        try
        {
            addBrk.parse(currentLine);
            mBreakpointsList.push_back(addBrk);
        }
        catch(std::exception)
        {
        }
    }
}

void Gdb::updateLocalVariables()
{
    QStringList locals = getLocalVar();
    mVariablesList.clear();
    for(auto i : locals)
    {
        Variable var(i, getVarType(i), getVarContent(i)); //todo
        mVariablesList.push_back(var);
        var.getSubVariables();
    }
}

std::vector<Breakpoint> Gdb::getBreakpoints() const
{
    return mBreakpointsList;
}

std::vector<Variable> Gdb::getLocalVariables() const
{
    return mVariablesList;
}

QString Gdb::getVarContent(const QString& var)
{
    write(QByteArray("print ").append(var));
    QProcess::waitForReadyRead(1000);
    QRegExp errorMatch("\\^done");
    if(errorMatch.indexIn(mBuffer) == -1)
    {
        throw std::exception("Error while var reading");
    }
    QRegExp content("=\\s.*\\^done");
    QRegExp clean("[\\\\|\|\"|~]");
//    QRegExp clean("\\\\n");
    qDebug() << ((content.indexIn(mBuffer) == -1) ? "Nothing captured" : "Captured smth succsesfully");
    QString res = content.cap().replace(clean, "").replace("^done", "").trimmed();
    res.resize(res.size()-1);
    auto lst = res.split('\n');
    for(QString& i : lst)
    {
        i = i.trimmed();
    }
    QString withoutLines = lst.join("");
    withoutLines.remove(0, 2);
    return withoutLines;

}

QString Gdb::getVarType(const QString &variable)
{   //finds and returns type of variable $variable$
    write(QByteArray("whatis ").append(variable));
    QProcess::waitForReadyRead(1000);
    QRegExp findType("type\\s=\\s[\\w:]+");
    if(findType.indexIn(mBuffer) == -1)
    {
        return QString();
    }
    QString type = findType.cap();
    QString bareType = type.split('=')[1].trimmed();
    return bareType;
}

void Gdb::globalUpdate()
{
    updateBreakpointsList();
    updateLocalVariables();
}

void Gdb::slotReadStdOutput()
{
    readStdOutput();
}

void Gdb::slotReadErrOutput()
{
    readErrOutput();
}

void Gdb::slotReadLocalVar()
{
    mLocalVar = QProcess::readAll();
}
