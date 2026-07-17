/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */

#include <moduleBasics/plot.h>

Plot::Plot(const std::shared_ptr<RuntimeIO>& rtIO, std::string plotName) : rtIO_(rtIO), plotName_(plotName) {}

void Plot::generatePlotData(fs::path data) {
  data_ = data;
  std::ofstream dataStream(data_.string());
  if (dataStream.is_open()) {
    dataStream << dataBody_.str() << std::endl;
    dataStream.close();
  } else {
    myRuntimeInfo->err << "Could not open - " << data_.string() << " ... - plotdata generated" << std::endl;
  }
}

/**
 * \brief Generate Plot Script
 *
 * \param script scriptname + path
 * \param format format
 * \param delimiter delimiter
 * \param xtraLinStyles additional linestyles -> bool
 * \param doubleSize double size -> bool
 * \param columnWidth columnwidth
 */
void Plot::generatePlotScript(fs::path script, const std::string format, std::string delimiter,
                              const bool xtraLinStyles, const bool doubleSize, double columnWidth) {
  script_ = script;
  generatePlotScriptHeader_(format, delimiter, xtraLinStyles, doubleSize, columnWidth);
  if (script_.extension().string().compare(".plt") == 0) {
    std::ofstream scriptStream(script_.string());
    if (scriptStream.is_open()) {
      scriptStream << scriptHeader_.str() << scriptBody_.str() << scriptFooter_.str() << std::endl;
      scriptStream.close();
    } else {
      myRuntimeInfo->err << "Could not open - " << script_.string() << " ... - no plotscript generated" << std::endl;
    }
  } else {
    myRuntimeInfo->err << "Name extension for script incorrect ... no plotscript generated" << std::endl;
  }
}

/**
 * \brief Generates Plot Scritp header
 *
 * \param format current format (e.g. 4:3)
 * \param delimiter datafile seperator
 * \param xtraLinStyles additional linestyles -> bool
 * \param doubleSize double size -> bool
 * \param columnWidth desired columnwidth
 */
void Plot::generatePlotScriptHeader_(const std::string format, std::string delimiter, const bool xtraLinStyles,
                                     const bool doubleSize, double columnWidth) {
  double width = atof((format.substr(0, format.find(':'))).c_str());
  double height = atof((format.substr(format.find(':') + 1, format.size() - 1)).c_str());
  double factor = width / height;
  double scaleFactor = doubleSize ? 2.0 : 1.0;

  scriptHeader_ << "######## Plotscript-Header ########" << std::endl
                << "reset" << std::endl
                << "set terminal svg size " << scaleFactor * columnWidth * 1.3 << ","
                << scaleFactor * columnWidth / factor * 1.3 << " enhanced font \"Times, 16\"" << std::endl
                << std::endl;
  if (delimiter != "none") {
    scriptHeader_ << "set datafile separator \"" << delimiter << "\"" << std::endl << std::endl;
  }
  scriptHeader_
      << std::endl
      << "# Dashtypes can be set by value (1-5), string \".\", \"-\", \"_\", or pattern (s1,e1,s2,e2,s3,e3,s4,e4) "
      << "with 1-4 numerical pairs of <solid length>s and <empty space length>e" << std::endl
      << "#     - black/grey -" << std::endl
      << "set style line  1  dt  1     lc rgb \"black\"      lw 2 pt 1  ps 0.75#black" << std::endl
      << "set style line  2  dt  2     lc rgb \"#696969\"    lw 2 pt 1  ps 0.75#DimGrey" << std::endl
      << "set style line  3  dt  3     lc rgb \"#BEBEBE\"    lw 2 pt 1  ps 0.75#Grey" << std::endl
      << "set style line  4  dt  4     lc rgb \"#D3D3D3\"    lw 2 pt 1  ps 0.75#LightGrey" << std::endl
      << "set style line  5  dt  5     lc rgb \"#708090\"    lw 2 pt 1  ps 0.75#SlateGrey" << std::endl
      << "set style line  6  dt \"_\"   lc rgb \"#9FB6CD\"    lw 2 pt 5  ps 0.75#SlateGrey3" << std::endl
      << "set style line  7  dt \"-\"   lc rgb \"#C6E2FF\"    lw 2 pt 6  ps 0.75#SlateGrey1" << std::endl
      << "set style line  8  dt \"._\"  lc rgb \"#528b8b\"    lw 2 pt 2  ps 0.75#DarkSlateGray4" << std::endl
      << "set style line  9  dt \"..-\"  lc rgb \"#79CDCD\"    lw 2 pt 3  ps 0.75#DarkSlateGray3" << std::endl;
  if (xtraLinStyles) {
    scriptHeader_ << std::endl
                  << "#     - blue -" << std::endl
                  << "set style line 11  dt  1     lc rgb \"#104E8B\"    lw 2 pt 8  ps 0.75#DogerBlue4" << std::endl
                  << "set style line 12  dt  2     lc rgb \"#473C8B\"    lw 2 pt 8  ps 0.75#SlateBlue4" << std::endl
                  << "set style line 13  dt  3     lc rgb \"#3A5FCD\"    lw 2 pt 8  ps 0.75#RoyalBlue3" << std::endl
                  << "set style line 14  dt  4     lc rgb \"#0000FF\"    lw 2 pt 8  ps 0.75#Blue" << std::endl
                  << "set style line 15  dt  5     lc rgb \"#4682B4\"    lw 2 pt 8  ps 0.75#SteelBlue" << std::endl
                  << "set style line 16  dt \"_\"   lc rgb \"#4F94CD\"    lw 2 pt 8  ps 0.75#SteelBlue3" << std::endl
                  << "set style line 17  dt \"-\"   lc rgb \"#6CA6CD\"    lw 2 pt 8  ps 0.75#SkyBlue3" << std::endl
                  << "set style line 18  dt \"._\"  lc rgb \"#63B8FF\"    lw 2 pt 8  ps 0.75#SteelBlue1" << std::endl
                  << "set style line 19  dt \"..-\"  lc rgb \"#CAE1FF\"    lw 2 pt 8  ps 0.75#LightSteelBlue1"
                  << std::endl
                  << std::endl
                  << "#     - red -" << std::endl
                  << "set style line 21  dt  1     lc rgb \"#B22222\"    lw 2 pt 7  ps 0.75#firebrick" << std::endl
                  << "set style line 22  dt  2     lc rgb \"#CD0000\"    lw 2 pt 7  ps 0.75#red3" << std::endl
                  << "set style line 23  dt  3     lc rgb \"#FF0000\"    lw 2 pt 7  ps 0.75#red" << std::endl
                  << "set style line 24  dt  4     lc rgb \"#FF3030\"    lw 2 pt 7  ps 0.75#firebrick1" << std::endl
                  << "set style line 25  dt  5     lc rgb \"#FF4500\"    lw 2 pt 7  ps 0.75#orangered" << std::endl
                  << "set style line 26  dt \"_\"   lc rgb \"#FF6347\"    lw 2 pt 7  ps 0.75#tomato" << std::endl
                  << "set style line 27  dt \"-\"   lc rgb \"#FF6A6A\"    lw 2 pt 7  ps 0.75#IndianRed1" << std::endl;
  }
  scriptHeader_ << std::endl
                << "set key font \"Times, 12\" samplen 1 width -2" << std::endl
                << "###################################" << std::endl
                << std::endl;
}

void Plot::call_gnuplot(fs::path script, std::string additionalArguments) {
  if (plotName_.empty()) {
    additionalArguments += "set output '" + fs::path(rtIO_->getPlotDir() + "/" + plotName_ + ".svg").string() + "'";
    myRuntimeInfo->debug << "Gnuplot set output: " << additionalArguments << std::endl;
  }

  if (fs::exists(script)) {
    std::string plotCmd = "gnuplot -e \"" + additionalArguments + "\"" + " " + script.string();
    handleChildProcess(plotCmd, "");
  } else {
    myRuntimeInfo->err << "Plot-File '" << script.string() << "' does not exist. Cannot plot!" << std::endl;
  }
}

void Plot::convert_plot_() {
  myRuntimeInfo->debug << " Convert " << plotName_ << " to PDF..." << std::endl;
#ifdef _WIN32
  std::string convertCmd =
      "inkscape -D -z --file=" + plotName_ + ".svg --export-pdf=" + svg_.filename().string() + ".pdf --export-latex";
#else
  std::string convertCmd = "inkscape -D --export-type=pdf --export-latex " + plotName_ + ".svg >/dev/null 2>&1";
#endif  // _WIN32
  handleChildProcess(convertCmd, "");
}
