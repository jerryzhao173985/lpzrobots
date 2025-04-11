#include "gnuplot.h"
#include "stl_adds.h"
#include <stdio.h>
#include <locale.h> // need to set LC_NUMERIC to have a '.' in the numbers piped to gnuplot
#include <QDebug>   // For debug output
#include <QTextStream> // For writing data to process
#include <QThread>    // For msleep

Gnuplot::Gnuplot(const PlotInfo* plotInfo)
  : plotInfo(plotInfo), process(nullptr) {
    // qDebug() << "Gnuplot::Gnuplot() - Constructor";
};

Gnuplot::~Gnuplot(){
  // qDebug() << "Gnuplot::~Gnuplot() - Destructor";
  close();
}

bool Gnuplot::init(const QString& gnuplotcmd, int w, int h, int x, int y){
  // qDebug() << "Gnuplot::init() - Cmd:" << gnuplotcmd << "Size:" << w << h << "Pos:" << x << y;
  windowWidth = w;
  windowHeight = h;
  windowX = x;
  windowY = y;
  return open(gnuplotcmd, w, h, x, y);
}

bool Gnuplot::open(const QString& gnuplotcmd, int w, int h, int x, int y){
  // qDebug() << "Gnuplot::open() - START";
  if (process && process->state() != QProcess::NotRunning) {
    qWarning() << "Gnuplot process is already running or starting.";
    return false;
  }

  close(); // Ensure any previous process is closed

  process = new QProcess();
  // qDebug() << "Gnuplot::open() - QProcess created";

  // Connect signals for error handling and process state monitoring
  QObject::connect(process, &QProcess::errorOccurred, 
                 [this](QProcess::ProcessError error) {
    qWarning() << "Gnuplot process error:" << error << process->errorString();
  });
  
  QObject::connect(process, &QProcess::readyReadStandardError, 
                 [this]() {
    QString errors = QString::fromUtf8(process->readAllStandardError());
    if (!errors.isEmpty()) {
      qWarning() << "Gnuplot stderr:" << errors;
    }
  });
  
  QObject::connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 
                 [this](int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "Gnuplot process exited with code:" << exitCode 
               << "status:" << exitStatus;
  });

  QStringList args;
  args << "-persist"; // Keep window open after parent exits

  setlocale(LC_NUMERIC, "C"); // set us type output for numbers

  // qDebug() << "Gnuplot::open() - Starting process:" << gnuplotcmd << args;
  process->start(gnuplotcmd, args);

  if (!process->waitForStarted(5000)) { // 5 sec timeout
    qWarning() << "Failed to start gnuplot:" << process->errorString();
    delete process;
    process = nullptr;
    // qDebug() << "Gnuplot::open() - FAILED to start";
    return false;
  }
  
  // qDebug() << "Gnuplot::open() - Process started successfully. PID:" << process->processId();
  
  // Try different terminal types in order of preference
  tryTerminals(w, h, x, y);  // Pass window dimensions to tryTerminals
  
  return true;
}

void Gnuplot::tryTerminals(int w, int h, int x, int y) {
  // List of terminal types to try, in order of preference
  // Note: some gnuplot builds might only have certain terminals available
  QStringList terminals = {
    "wxt",    // wxWidgets terminal
    "x11",    // Classic X11 terminal
    "qt",     // Modern Qt terminal // sudo apt-get install gnuplot-qt qt5-default
    "dumb"    // Last resort - text mode
  };
  
  for (const QString& term : terminals) {
    // Try setting the terminal
    if (term == "qt") {
      // For Qt terminal, set window size and position
      QString sizeCmd = QString("set terminal qt size %1,%2 enhanced").arg(w).arg(h);
      command(sizeCmd);
      
      // Set position if specified (x,y != -1)
      if (x != -1 && y != -1) {
        QString positionCmd = QString("set terminal qt position %1,%2").arg(x).arg(y);
        command(positionCmd);
      }
      
      command("set term qt font \"Arial,9\"");
    } else if (term == "wxt") {
      QString wxtCmd = QString("set terminal wxt size %1,%2 enhanced").arg(w).arg(h);
      command(wxtCmd);
      if (x != -1 && y != -1) {
        QString positionCmd = QString("set terminal wxt position %1,%2").arg(x).arg(y);
        command(positionCmd);
      }
    } else if (term == "x11") {
      QString x11Cmd = QString("set terminal x11 size %1,%2 enhanced").arg(w).arg(h);
      command(x11Cmd);
    } else {
      QString cmd = QString("set terminal %1").arg(term);
      command(cmd);
    }
    
    // Set basic options for readability
    command("set style data lines");
    command("set grid");
    command("set zeroaxis");
    
    // Send a test plot to check if terminal works
    command("plot sin(x) title 'Test Plot'");
    
    // Wait a bit for the terminal to initialize
    QThread::msleep(200); // Give it a bit more time
    
    // Check if process is still running
    if (process && process->state() == QProcess::Running) {
      // qDebug() << "Gnuplot::tryTerminals - Successfully using terminal:" << term;
      
      // If we're using qt, apply additional settings
      if (term == "qt") {
        command("set term qt enhanced persist raise");
        command("set termoption dash"); // Enable dashed lines
      }
      
      return; // Success with this terminal
    } else {
      qWarning() << "Gnuplot::tryTerminals - Terminal" << term << "failed, trying next.";
      
      // Re-create process if it crashed
      if (!process || process->state() != QProcess::Running) {
        if (process) {
          delete process;
        }
        process = new QProcess();
        
        // Connect basic signals
        QObject::connect(process, &QProcess::errorOccurred, 
                     [this](QProcess::ProcessError error) {
          qWarning() << "Gnuplot process error:" << error << process->errorString();
        });
        
        process->start("gnuplot", QStringList() << "-persist");
        
        if (!process->waitForStarted(1000)) {
          qWarning() << "Failed to restart gnuplot process";
          return; // Give up if we can't restart
        }
      }
    }
  }
  
  qWarning() << "Gnuplot::tryTerminals - Failed to find a working terminal.";
}

void Gnuplot::close(){
  if (process) {
    // qDebug() << "Gnuplot::close() - Closing process. Current state:" << process->state();
    if (process->state() == QProcess::Running) {
      process->terminate(); // Ask nicely first
      if (!process->waitForFinished(1000)) { // Wait 1 sec
        qWarning() << "Gnuplot did not terminate gracefully, killing.";
        process->kill();
        process->waitForFinished(500);
      }
    }
    // qDebug() << "Gnuplot::close() - Deleting process object.";
    delete process;
    process = nullptr;
  }
}


/** send arbitrary command to gnuplot.
    like "set zeroaxis" or other stuff */
void Gnuplot::command(const QString& cmd){
  if (!process) {
    qWarning() << "Gnuplot::command() - No process exists, cannot send command:" << cmd;
    return;
  }
  
  if (process->state() != QProcess::Running) {
    qWarning() << "Gnuplot::command() - Process not running, cannot send command:" << cmd;
    return;
  }
  
  QByteArray data = cmd.toUtf8() + "\n"; // Ensure newline
  qint64 bytesWritten = process->write(data);
  if (bytesWritten == -1) {
    qWarning() << "Gnuplot::command() - Error writing to process:" << process->errorString();
  } else {
    // Force flush the write buffer
    process->waitForBytesWritten(100); // 100ms timeout
  }
}


/** print buffer content to stream */
void plotDataSet(QTextStream& stream, const ChannelVals& vals){
  for (double v : vals) {
    stream << v << " ";
  }
  stream << "\n";
};


/** make gnuplot plot selected content of data buffers */
QString Gnuplot::plotCmd(const QString& file, int start, int end){
  if (!plotInfo) return QString();
  const std::list<int>& vc = plotInfo->getVisibleChannels();
  if(vc.size()==0) return QString();
  QStringList buffer;
  bool first=true;
  const ChannelData& cd = plotInfo->getChannelData();
  QString range;
  if(start!=-1 && end!=-1){
    range=QString(" every ::%1::%2 ").arg(start).arg(end);
  }

  for (int i : vc){
    if(first){
      buffer << "plot '" << (file.isEmpty() ? "-" : file)  << "' ";
      first=false;
    } else {
      buffer << ", '' ";
    }
    buffer << range;
    if(!file.isEmpty()){
      if(plotInfo->getUseReference1()){
        buffer << QString(" u %1:%2 ").arg(plotInfo->getReference1()+1).arg(i+1);
      }else{
        // TODO add reference2!
        buffer << QString(" u %1 ").arg(i+1);
      }
    }
    // Use channel name if available, otherwise index
    const ChannelName& name = cd.getChannelName(i);
    buffer << "t '" << (name.isEmpty() ? QString::number(i) : name) << "'";
    const auto& channelInfos = plotInfo->getChannelInfos();
    if(i >= 0 && i < channelInfos.size() && channelInfos[i].style != PS_DEFAULT)
      buffer << " w " << channelInfos[i].getStyleString();
  }
  return buffer.join(QString());
}



/** make gnuplot plot selected content of data buffers */
void Gnuplot::plot(int historyLimit){
  if (!process || process->state() != QProcess::Running) {
      qWarning() << "Gnuplot::plot() - Process not running, cannot plot.";
      return;
  }
  if (!plotInfo || !plotInfo->getIsVisible()) {
       // This can be normal if a window is disabled in config
       // qDebug() << "Gnuplot::plot() - PlotInfo not valid or not visible, skipping plot.";
      return;
  }

  const ChannelData& cd = plotInfo->getChannelData();
  const std::list<int>& vc = plotInfo->getVisibleChannels();
  if(vc.empty()) { // Use empty() for std::list
       // qDebug() << "Gnuplot::plot() - No visible channels, skipping plot.";
      return;
  }

  QString plotCommandStr = plotCmd();
  if (plotCommandStr.isEmpty()) {
       // qDebug() << "Gnuplot::plot() - Generated plot command is empty, skipping plot.";
      return;
  }

  // Apply a reasonable limit if necessary
  int actualLimit = historyLimit;
  if (actualLimit <= 0 || actualLimit > 1000) {
    actualLimit = 1000; // Default reasonable limit
    // qDebug() << "Gnuplot::plot() - Using default history limit:" << actualLimit;
  }

  // Send the plot command to gnuplot
  // qDebug() << "Gnuplot::plot() - Sending plot command:" << plotCommandStr;
  command(plotCommandStr);

  // Use a local QTextStream that doesn't keep the process blocked
  QByteArray dataBuffer;
  QTextStream stream(&dataBuffer);
  stream.setRealNumberPrecision(10); // Or desired precision
  stream.setLocale(QLocale::C);      // Ensure '.' as decimal separator

  if(plotInfo->getUseReference1()){
    std::list<int> visibles(vc);
    visibles.push_front(plotInfo->getReference1());
    // Use historyLimit instead of 0
    const QVector<ChannelVals>& vals = cd.getHistory(visibles, actualLimit); 
    // qDebug() << "Gnuplot::plot() - Plotting with reference1. History size:" << vals.size() << "Limit:" << actualLimit;
    int len = visibles.size();
    for(int k=1; k< len; k++){
      for (const auto& v : vals){
         if (v.size() > k && v.size() > 0) { // Check bounds
            stream << v[0] << " " << v[k] << "\n";
         }
      }
      stream << "e\n";
    }
  } else {
    // Use historyLimit instead of 0
    const QVector<ChannelVals>& vals = cd.getHistory(vc, actualLimit); 
    // qDebug() << "Gnuplot::plot() - Plotting without reference. History size:" << vals.size() << "Limit:" << actualLimit;
    int len = vc.size();
    for(int k=0; k< len; k++){
       for (const auto& v : vals){
          if (v.size() > k) { // Check bounds
             stream << v[k] << "\n";
          }
       }
       stream << "e\n";
    }
  }
  
  // Flush the stream to ensure data is in the buffer
  stream.flush();
  
  // Write data in smaller chunks to avoid blocking
  const int chunkSize = 4096; // 4KB chunks
  QByteArray data = dataBuffer;
  // qDebug() << "Gnuplot::plot() - Sending" << data.size() << "bytes of data to gnuplot.";
  
  for (int i = 0; i < data.size(); i += chunkSize) {
    QByteArray chunk = data.mid(i, qMin(chunkSize, data.size() - i));
    process->write(chunk);
    // Don't wait for bytes written, just continue
  }
  
  // qDebug() << "Gnuplot::plot() - Data sent to gnuplot.";
};
